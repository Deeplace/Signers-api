<?php

class MSign extends Signer {

  protected $wsdl_path = '';
  protected $wsdl_verify_path = '';

  protected $requestId;
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

    if(empty($this->wsdl_sign_path)) {
      $this->wsdl_sign_path = variable_get('signer_msign_wsdl_path', '');
    }
  }

  public function sign() {
    try {
      $connect = new SoapClient(
        $this->wsdl_sign_path,
          array(
            'cache_wsdl' => WSDL_CACHE_NONE,
            'local_cert' => realpath(variable_get('signer_msign_authentication_certificate', ''))
          )
        );
      if(!$connect) {
        throw new SignerException("Cant sign document", "Cant connect to server {$this->wsdl_sign_path}");
      }

      $data_to_be_sign = pack('H*',$this->hash);

      $param = array(
        'request' => array(
          'ShortContentDescription' => t('Sign Agepi document'),
          'ContentType' => 'Hash',
          'Contents' => array(
            array(
              'MultipleSignatures' => FALSE,
              'Content' => $data_to_be_sign
            )
          ),
        )
      );

      $res = $connect->PostSignRequest($param);

      if(!isset($res->PostSignRequestResult)) {
        throw new SignerException("Cant sign document", "Soap Fault. Detail " . var_export($connect->__getLastResponse(), 1));
      }

      $this->createTransaction($res->PostSignRequestResult);

      return array(
        'requestId' => $res->PostSignRequestResult,
      );
    }
    catch(SoapFault $e) {
      throw new SignerException("Cant sign document", "Soap Fault. Exception: " . $e->getMessage());
    }
  }

  public function createTransaction($requestId) {
    $transaction = new stdClass();
    $transaction->request_id = $requestId;
    $transaction->hash_fid = $this->fid;
    $transaction->created = time();
    $transaction->updated = $transaction->created;
    $transaction->info = serialize($this->getInfo());
    $transaction->uid = $this->uid;
    $transaction->status = '';
    drupal_write_record('msign_transaction', $transaction);
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

  public function loadRequestData($request_id) {
    $connect = new SoapClient(
      $this->wsdl_sign_path,
      array(
        'cache_wsdl' => WSDL_CACHE_NONE,
        'local_cert' => realpath(variable_get('signer_msign_authentication_certificate', ''))
      )
    );

    if(!$connect) {
      throw new SignerException("Cant verify document status.", "Cant verify document status . Cant find wsdl on path {$this->wsdl_sign_path}");
    }

    $res = $connect->GetSignResponse(array('requestID' => $request_id));

    if (!isset($res->GetSignResponseResult->Results->SignResult)) {
      throw new SignerException("Cant verify document status.", "Cant verify document status . Request_id:{$request_id}. Soap Fault. Detail " . var_export($connect->__getLastResponse(), 1), WATCHDOG_INFO);
    }

    $this->signatureData = $res->GetSignResponseResult->Results->SignResult->Signature;
    $this->setSignatureFromData();

    return $res->GetSignResponseResult;
  }
}
