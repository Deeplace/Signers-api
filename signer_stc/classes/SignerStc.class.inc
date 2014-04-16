<?php
class SignerStc extends Signer {
  const VALID_SIGNATURE = "ValidSignature";

  public function __construct($param = NULL) {
    parent::__construct($param);
    if(!empty($this->rsa_sign)) {
      $this->signed_data = $this->rsa_sign;
      $this->rsa_sign = $this->getSignature();
    }
  }

  public function setData($arg) {
    parent::setData($arg);
    if(!empty($this->rsa_sign)) {
      $this->rsa_sign = $this->getSignature();
    }
  }

  public function prepareSignData($uri) {
    $hash = $this->createHash($uri);
    return array('hash' => array('hash' => $hash, 'base64' => base64_encode(pack('H*', $hash))));
  }

  public function save() {
    parent::save();

    $document_uri = variable_get('signature_data_save_place', 'db://signatures_data/?id=') . $this->id;

    if (file_put_contents($document_uri, $this->signed_data) === FALSE) {
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

  public function validate() {
    parent::validate();
    $this->checkCertificate();
    $this->verify();
  }

  public function verify() {
    try {
      $client = new SoapClient(variable_get('verification_service_wsdl_path', drupal_get_path('module', 'signer_stc').'/classes/verify.wsdl'), array('cache_wsdl' => WSDL_CACHE_NONE, 'trace' => 1, 'soap_version' => SOAP_1_2));

      $parameters = array(
        'ServiceId' => variable_get('verification_service_id', 'Deeplace-test-01'),
        'DataContentType' => 'Binary',
        'SignatureFormat' => 'XadEs',
        'SignatureType' => 'Detached',
        'Signature' => $this->signed_data,
        'DataToBeVerify' => pack('H*', $this->hash),
      );

      $res = $client->Verify(
        array('request' => $parameters)
      );

      if ($res->VerifyResult->StatusCode == self::VALID_SIGNATURE) {
        return TRUE;
      }
      else {
        throw new SignerException("Document failed validation", "Signature validation failed. Status:{$res->VerifyResult->StatusCode}, details: {$res->VerifyResult->StatusMessage} ");
      }
    } catch (SoapFault $e) {
      throw new SignerException("Document failed validation", "Soap service failed " . $e->getMessage());
    }
  }

  /**
   * Extracts signature from signed data
   */
  protected function getSignature() {
    $sign_data = simplexml_load_string($this->rsa_sign);
    $signature = (string)$sign_data->children('ds', true)->SignatureValue;

    return empty($signature) ? $this->rsa_sign : $signature;
  }

  public function unsign() {
    libxml_use_internal_errors();
    $cert_exist = FALSE;
    $orig_xml = new SimpleXMLElement($this->signed_data);
    $original_certificate = (string)$orig_xml->children('ds', true)->KeyInfo->X509Data->X509Certificate;

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
          if($original_certificate == (string)$xml->children('ds', true)->KeyInfo->X509Data->X509Certificate) {
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
    $orig_xml = new SimpleXMLElement($this->signed_data);
    $original_certificate = (string)$orig_xml->children('ds', true)->KeyInfo->X509Data->X509Certificate;
    $cert_data = openssl_x509_parse("-----BEGIN CERTIFICATE-----\n" . $original_certificate . "\n-----END CERTIFICATE-----");

    if(!$cert_data) {
      throw new SignerException("Cant validate certificate", "Cant parse certificate");
    }

    if($this->idnp == $cert_data['subject']['serialNumber']) {
      return TRUE;
    }

    throw new SignerException("IDNP specified in profile and IDNP specified in certificate are different", "IDNP specified in profile and IDNP specified in certificate are different");
  }
}
