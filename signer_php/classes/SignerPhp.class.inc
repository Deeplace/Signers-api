<?php
class Signer_PHP extends Signer {
  public $custom_validate_callbacks = array();

  public function validate() {
    parent::validate();
    foreach($this->custom_validate_callbacks as $v) {
      call_user_func($v, $this);
    }
  }

  public function verify($certificates_path) {
    if(!user_access('reception sign document', user_load($this->uid))) {
      throw new SignerException("Reception cant access to sign document", "Reception cant access to sign document");
    }

    if(!file_exists($certificates_path)) {
      throw new SignerException('Not found certificate for your key. Please generate ',
        'Certificate file '.$certificates_path.' not found');
    }

    $certificate = file_get_contents($certificates_path);
    $certificate_data = openssl_x509_parse($certificate);

    if($certificate_data['validTo_time_t'] < time()) {
      user_certificate_revocate_certificate($certificates_path, $certificates_path);
      throw new SignerException('Reception certificate was expired.', 'Reception certificate was expired.');
    }

    $publickeyid = openssl_get_publickey($certificate);

    if($publickeyid === FALSE) {
      throw new SignerException('Error occured, try again later.', 'Can not load openssl resourse for certificate ' . $certificates_path);
    }

    // state whether signature is okay or not
    $result = openssl_verify($this->hash, $this->rsa_sign, $publickeyid);
    // Free the key from memory
    openssl_free_key($publickeyid);

    if($result !== 1) {
      throw new SignerException("Reception signature is not valid", "Reception signature is not valid");
    }

    return TRUE;
  }

  /**
   * Returns JSON data to JavaScript to perform sign process
   */
  public function sign($privkey) {
    if(!$privkeyid = openssl_get_privatekey($privkey)) {
      throw new SignerException('private key can not be loaded', 'private key can not be loaded');
    }
    openssl_sign($this->hash, $signed_data, $privkeyid);
    openssl_free_key($privkeyid);
    $this->rsa_sign = $signed_data;

    return TRUE;
  }
}
