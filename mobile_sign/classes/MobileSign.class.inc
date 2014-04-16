<?php

class MobileSign extends Signer {
  const OUSTANDING_TRANSACTION = 'OustandingTransaction';
  const VALID_SIGNATURE = 'ValidSignature';
  const RESPONSE_REQUEST_OK = 'RequestOk';
  const INTERNAL_ERROR = 'InternalERROR';
  const WRONG_IDNP = 'WrongIDNP';

  const SIGN_RESPONSE_XML_TEMPLATE = '<?xml version="1.0" encoding="UTF-8"?><ns1:NotifyMobileSignatureResponse xsi:schemaLocation="http://msignature.cts.md NotifyResponse.xsd" xmlns:ns1="http://msignature.cts.md" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"><ns1:StatusCode>:statusCode</ns1:StatusCode><ns1:StatusMessage>:statusMessage</ns1:StatusMessage></ns1:NotifyMobileSignatureResponse>';
  protected $wsdl_sign_path = '';
  protected $wsdl_verify_path = '';
  protected $phone = '';
  protected $serviceId = '';
  protected $transactionId;
  protected $phoneDisplayMessage = '';
  protected $userDisplayMessage = '';
  protected $verification_code = '';
  protected $signatureData = '';

  function __construct($param) {
    parent::__construct($param);

    if(!empty($this->signature_fid)) {
      $file = file_load($this->signature_fid);
      if($file) {
        $this->signatureData = file_get_contents($file->uri);
      }
    }

    if(!empty($this->signatureData) && empty($this->rsa_sign)) {
      $this->setSignatureFromData();
    }

    $module_path = drupal_get_path('module', 'mobile_sign');
    if(empty($this->wsdl_sign_path)) {
      $this->wsdl_sign_path = variable_get('mobile_sign_wsdl_path', $module_path . '/wsdl/sign.wsdl');
    }

    if(empty($this->wsdl_verify_path)) {
      $this->wsdl_verify_path = variable_get('mobile_verify_wsdl_path', $module_path . '/wsdl/verify.wsdl');
    }

    if(empty($this->serviceId)) {
      $this->serviceId = variable_get('mobile_sign_service_id', '');
    }

    if(empty($this->phoneDisplayMessage)) {
      $this->phoneDisplayMessage = variable_get('mobile_sign_phone_display_message', 'Verification code !code');
    }

    if(empty($this->userDisplayMessage)) {
      $this->userDisplayMessage = variable_get('mobile_sign_user_display_message', 'Verification code !code');
    }
  }

  public function sign() {
    try {
      $connect = new SoapClient($this->wsdl_sign_path, array('cache_wsdl' => WSDL_CACHE_NONE, 'trace' => 1, 'soap_version' => SOAP_1_2));
      if(!$connect) {
        throw new SignerException("Cant sign document", "Cant find wsdl on path {$this->wsdl_sign_path}");
      }
      $data_to_be_sign = pack('H*',$this->hash);
      $data_to_be_sign = base64_encode($data_to_be_sign);

      $parameters =  array(
        'ServiceId' => $this->serviceId,
        'TransactionId' => $this->generateTransactionId(),
        'RoleId' => 0,
        'DataToBeSign' => $data_to_be_sign,
        'DataContentType' => 'Hash',
        'SignatureFormat' => 'XadEsT',
        'SignatureType' => 'Enveloping',
        'HandlingMessageType' => 'Async',
        'CallBackUrl' => variable_get('mobile_sign_callback_url', url('mobile_id/response', array('absolute' => TRUE))),
        'Msisdn' => $this->phone,
        'DataToBeDisplay' => strtr($this->phoneDisplayMessage, array('!code' => $this->generateVerificationCode())),
        'SignAs' => 'Personal'
      );

      $res = $connect->Sign(
        array('request' => $parameters)
      );

      watchdog('mobile_sign_send_data', 'Send data to sign. Data: !data', array('!data' => var_export($parameters, 1)), WATCHDOG_INFO);

      if(!isset($res->SignResult->StatusCode) || $res->SignResult->StatusCode != self::OUSTANDING_TRANSACTION) {
        throw new SignerException("Cant sign document", "Soap Fault. Detail " . var_export($connect->__getLastResponse(), 1));
      }

      $this->createTransaction();

      return array(
        'transactionId' => $this->transactionId,
        'successMessage' => strtr($this->userDisplayMessage, array('!code' => $this->verification_code)),
      );
    }
    catch(SoapFault $e) {
      throw new SignerException("Cant sign document", "Soap Fault. Exception: " . $e->getMessage());
    }
  }

  public function generateTransactionId() {
    $this->transactionId = uniqid();
    $exist = db_select('mobile_id_transaction', 'mt')
      ->fields('mt', array('transactionId'))
      ->condition('mt.transactionId', $this->transactionId)
      ->execute()->fetchField();

    if($exist) {
      $this->generateTransactionId();
    }

    return $this->transactionId;
  }

  public function generateVerificationCode() {
    $this->verification_code = str_pad(rand(0, 99999), 6, '0', STR_PAD_LEFT);
    return $this->verification_code;
  }

  public function createTransaction() {
    $transaction = new stdClass();
    $transaction->transactionId = $this->transactionId;
    $transaction->hash_fid = $this->fid;
    $transaction->status = self::OUSTANDING_TRANSACTION;
    $transaction->phone = $this->phone;
    $transaction->verification_code = $this->verification_code;
    $transaction->created = time();
    $transaction->updated = $transaction->created;
    $transaction->info = serialize($this->getInfo());
    $transaction->uid = $this->uid;
    $transaction->user_message = '';
    drupal_write_record('mobile_id_transaction', $transaction);
  }

  public function save() {
    parent::save();

    $document_uri = variable_get('signature_data_save_place', 'db://signatures_data/?id=') . $this->id;

    if (file_put_contents($document_uri, $this->signatureData) === FALSE) {
      throw new SignerException("Document failed signed", "can not save signature_fid");
    }

    global $user;
    $file = new stdClass();
    $file->uri = $document_uri;
    $file->status = FILE_STATUS_PERMANENT;
    $file->uid = $user->uid;
    file_save($file);
    $this->signature_fid = $file->fid;

    drupal_write_record('signer_signatures', $this, 'id');

    return $this;
  }

  private function setSignatureFromData() {
    if(empty($this->signatureData)) return;
    libxml_use_internal_errors();
    $xml = new SimpleXMLElement($this->signatureData);
    $this->rsa_sign = (string)$xml->children('http://www.w3.org/2000/09/xmldsig#', FALSE)->SignatureValue;
  }

  private function getInfo() {
    return array(
      'fid' => $this->fid,
      'module' => $this->module,
      'hash' => $this->hash,
      'uid' => $this->uid,
      'operation' => $this->operation,
      'sign_type' => $this->sign_type,
      'additional_data' => $this->additional_data,
    );
  }

  public function unsign() {
    libxml_use_internal_errors();
    $cert_exist = FALSE;

    $orig_xml = new SimpleXMLElement($this->signatureData);
    $original_certificate = (string)$orig_xml->children('http://www.w3.org/2000/09/xmldsig#', FALSE)->KeyInfo->X509Data->X509Certificate;

    if(empty($original_certificate)) {
      throw new SignerException("Cant unsign document", "Cant find certificate in received xml with fid {$this->fid} and module {$this->module}.");
    }
    $signature = FALSE;

    //Select all signatures for this document
    $result = db_select('signer_signatures', 'ss')
      ->fields('ss')
      ->condition('fid', $this->fid)
      ->condition('module', $this->module)
      ->execute();

    foreach($result as $obj) {
      try {
        $file = file_load($obj->signature_fid);
        if($file) {
          $xml_data = file_get_contents($file->uri);
          $xml = new SimpleXMLElement($xml_data);
          if($original_certificate == (string)$xml->children('http://www.w3.org/2000/09/xmldsig#', FALSE)->KeyInfo->X509Data->X509Certificate) {
            $cert_exist = TRUE;
            $signature = $obj;
            break;
          }
        }
      }
      catch (Exception $e) {
        continue;
      }
    }

    if(empty($cert_exist) || empty($signature)) {
      throw new SignerException("Cant find you signature in sign document", "Cant find signature for document with fid {$this->fid} and module {$this->module}.");
    }

    if($signature) {
      db_delete('signer_signatures')
        ->condition('id', $signature->id)
        ->execute();
    }

    $params = array(
      'fid' => $this->fid,
      'additional_data' => $this->additional_data,
      'id' => $this->id
    );

    if(!empty($signature)) {
      $file = file_load($signature->signature_fid);
      file_delete($file);
    }

    module_invoke($this->module, 'signer_remove_sign', $params);
  }

  public function verify() {
    try {
      $connect = new SoapClient($this->wsdl_verify_path, array('cache_wsdl' => WSDL_CACHE_NONE, 'trace' => 1, 'soap_version' => SOAP_1_2));
      if(!$connect) {
        throw new SignerException("Cant sign document", "Cant find wsdl on path {$this->wsdl_verify_path}");
      }

      $file = file_load($this->signature_fid);
      $signature = file_get_contents($file->uri);

      $signed_data = pack('H*',$this->hash);
      $signed_data = base64_encode($signed_data);

      $parameters =  array(
        'ServiceId' => $this->serviceId,
        'DataContentType' => 'Text',
        'SignatureFormat' => 'XadEsT',
        'SignatureType' => 'Detached',
        'Signature' => $signature,
        'DataToBeVerify' => $signed_data,
      );

      $res = $connect->Verify(
        array('request' => $parameters)
      );

      if(isset($res->SignResult->StatusCode) && $res->VerifyResult->StatusCode == self::VALID_SIGNATURE) {
        return TRUE;
      }
      else {
        return FALSE;
      }
    }
    catch(SoapFault $e) {
      throw new SignerException("Cant sign document", "Soap Fault. Exception: " . $e->getMessage());
    }
  }

  public function checkCertificate() {
    $orig_xml = new SimpleXMLElement($this->signatureData);

    $original_certificate = (string)$orig_xml->children('http://www.w3.org/2000/09/xmldsig#', FALSE)->KeyInfo->X509Data->X509Certificate;

    $cert_data = openssl_x509_parse("-----BEGIN CERTIFICATE-----\n" . $original_certificate . "\n-----END CERTIFICATE-----");

    if(!$cert_data) {
      return FALSE;
    }

    if($this->idnp == $cert_data['subject']['serialNumber']) {
      return TRUE;
    }

    return FALSE;
  }
}
