<?php
/**
 * Signer plugin.
 * @implements  Signer_Plugin
 */
class SignerJs extends Signer {
  public function __construct($param = NULL) {
    parent::__construct($param);
    if(!empty($this->rsa_sign)) {
      $this->rsa_sign = static::transformSignature($this->rsa_sign);
    }
  }

  public function setData($arg) {
    parent::setData($arg);
    if(!empty($this->rsa_sign)) {
      $this->rsa_sign = static::transformSignature($this->rsa_sign);
    }
  }

  public function validate() {
    parent::validate();
    $this->verify();
  }

  public function verify() {
    // Verify that signature is valid
    $certificates_path = rtrim(variable_get('user_certificates_path', ''), ' /');
    if (empty($this->idnp)) {
      throw new SignerException('IDNP is not set', 'IDNP is not set');
    }
    $first_letters = drupal_substr($this->idnp, 0, CERTIFIFICATE_IDNO_PREFIX_COUNT);
    $certificates_path .= '/' . $first_letters . '/' . $this->idnp . '.cer';

    if(!file_exists($certificates_path)) {
      throw new SignerException('Not found certificate for your key. Please generate ',
        'Certificate file '.$certificates_path.' not found');
    }

    $certificate = file_get_contents($certificates_path);
    $certificate_data = openssl_x509_parse($certificate);

    if($certificate_data['validTo_time_t'] < time()) {
      user_certificate_revocate_certificate($certificates_path, $certificates_path);
      throw new SignerException('You certificate was expired. Please generate'  . ' ' . l(t('new certificate'), 'user/create-certificate'), 'Certificate was expired.');
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
      throw new SignerException("Your signature is not valid", "Signature is not valid ");
    }

    return TRUE;
  }

  public static function transformSignature($signature) {
    $pad = strlen($signature) % 256;

    if($pad) {
      $pad = 256 - $pad;
      $signature = str_repeat('0', $pad) . $signature;
    }

    $signature = strtr($signature, array("\n" => '', "\r" => '', ));
    $signature = @pack("H*", $signature);

    return $signature;
  }
}
