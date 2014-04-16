<?php

abstract class Signer {
  const OPERATION_ADD = 'ADD';
  const OPERATION_REMOVE = 'REMOVE';

  /**
   * Sign id
   * @var integer
   */
  public $id = 0;

  /**
   * Sign data fid
   * @var integer
   */
  public $fid = 0;

  /**
   * Module that sign data
   * @var integer
   */
  public $module = 0;
  /**
   * data hash
   * @var string
   */
  public $hash = '';

  /**
   * Rsa sign data
   * @var string
   */
  public $rsa_sign = '';

  /**
   * Id of user that perorms signing operation
   * @var integer
   */

  public $uid = 0;

  /**
   * Fid of signature
   * @var string
   */
  public $signature_fid = 0;

  /**
   * Timestamp of signature
   * @var string
   */
  public $created = 0;

  /**
   * Operation ADD or REMOVE sign
   * @var string
   */
  public $operation = '';

  /**
   * Type of sign
   * @var string
   */
  public $sign_type = '';

  /**
   * @var string - IDNP of signer user
   */
  public $idnp = '';

  /**
   * Contains additional_data
   * @var array()
   */
  public $additional_data = array();

  public function __construct($param = NULL) {
    if (is_array($param) || is_object($param)) {
      $param = (array)$param;
      foreach($param as $k => $v) {
        if(isset($this->$k)) {
          if(is_array($v)) {
            $this->$k = array_filter($v);
          }
          else {
            $this->$k = $v;
          }
        }
      }
    }
    elseif (is_numeric($param)) {
      $this->id = $param;
      $this->load();
    }
  }

  public function load() {
    $signer = db_select('signer_signatures', 'ss')
      ->fields('ss')
      ->condition('id', $this->id)
      ->execute()->fetchObject();

    if (!$signer) {
      throw new Exception(t('Can\'t load signature object with id = @id', array('@id' => $this->id)));
    }

    foreach ($signer as $k => $v) {
      $key = (string) $k;
      if($key == 'id') continue;
      if(isset($this->$key)) {
        $this->$key = $v;
      }
    }

    if(!empty($this->additional_data)) {
      $this->additional_data = unserialize($this->additional_data);
    }
  }

  public function setData($arg) {
    if (is_numeric($arg)){
      $this->load($arg);
    }
    elseif (is_object($arg) || is_array($arg)) {
      $arg = (array)$arg;
      foreach ($arg as $key => $value) {
        if($key == 'id') continue;
        $this->$key = $value;
      }
    }
  }

  function validate() {
    $result = '';
    if(empty($this->rsa_sign)) {
      throw new SignerException("Document failed signed", "Document with id {$this->additional_data['document_id']} received null signed data. Module {$this->module}");
    }

    if(empty($this->hash)) {
      throw new SignerException("Document failed signed", "Document with id {$this->additional_data['document_id']} received null data for sign. Module - {$this->module}");
    }

    if (!user_access('sign documents', user_load($this->uid))) {
      throw new SignerException("You dont have permissions to sign document", "Document @doc_id can not be signed by current user. Permission denied. Result: uid - {$this->uid}. Module - {$this->module}");
    }

    $params = array(
      'rsa_sign' => $this->rsa_sign,
      'hash' => $this->hash,
      'fid' => $this->fid,
      'additional_data' => $this->additional_data,
      'uid' => $this->uid,
      'module' => $this->module
    );

    if (empty($this->module)) {
      throw new SignerException("Document failed signed", "Validation performed by module {$this->module} failed");
    }

    if($this->operation == static::OPERATION_ADD) {
      $result = module_invoke($this->module, 'sign_validate_data', $params);
    }
    elseif($this->operation == static::OPERATION_REMOVE) {
      $result = module_invoke($this->module, 'unsign_validate_data', $params);
    }

    if (empty($result)) {
      throw new SignerException("Document failed " . (($this->operation == static::OPERATION_ADD) ? 'signed' : 'unsign'), "Validation performed by module failed. Module {$this->module}");
    }
  }

  public function save() {
    $transaction = db_transaction();
    try {
      if ($this->id) {
        drupal_write_record('signer_signatures', $this, 'id');
      }
      else {
        $this->created = time();
        drupal_write_record('signer_signatures', $this);
      }

      $params = array(
        'rsa_sign' => $this->rsa_sign,
        'hash' => $this->hash,
        'fid' => $this->fid,
        'additional_data' => $this->additional_data,
        'id' => $this->id
      );

      $result = module_invoke($this->module, 'store_signed_data', $params);

      if (empty($result)) {
        throw new SignerException("Error Processing Request", "Saving signature performed by module failed");
      }
    }
    catch(SignerException $e) {
      $transaction->rollback();
      throw new SignerException($e->getMessage(), $e->getSystemMessage());
    }
    catch(Exception $e) {
      $transaction->rollback();
      throw new SignerException("Error Processing Request", $e->getMessage());
    }

    return $this;
  }

  public function prepareSignData($uri) {
    $this->createHash($uri);
    return array('hash' => $this->hash);
  }

  protected function createHash($uri) {
    if (!$this->hash = sha1_file($uri)) {
      throw new Exception('Can\'t generate hash');
    }
    return $this->hash;
  }

  /**
   * @param $plugin_name - class name for needed plugin
   */
  public static function getSigner($plugin_name, $param = array()) {
    $links = module_invoke_all('signer_plugin_class');
    foreach($links as $v) {
      if($v == $plugin_name) {
        return new $plugin_name($param);
      }
    }
    return FALSE;
  }

  public function unsign() {
    $signature_id = db_select('signer_signatures', 'ss')
      ->fields('ss', array('id'))
      ->condition('fid', $this->fid)
      ->condition('module', $this->module)
      ->condition('rsa_sign', $this->rsa_sign)
      ->execute()->fetchField();

    if(empty($signature_id)) {
      throw new SignerException("Cant find you signature in sign document", "Cant find signature for document with fid {$this->fid} and module {$this->module}.");
    }

    db_delete('signer_signatures')
      ->condition('id', $signature_id)
      ->execute();

    $params = array(
      'fid' => $this->fid,
      'additional_data' => $this->additional_data,
      'id' => $this->id
    );

    if(!empty($this->signature_fid)) {
      $file = file_load($this->signature_fid);
      file_delete($file);
    }

    module_invoke($this->module, 'signer_remove_sign', $params);
  }
}
