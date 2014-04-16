<?php

function signer_api_prepare_sign() {
  $result = FALSE;

  try {
    $uri = $_POST['uri'];
    $sign_plugin = $_POST['sign_plugin'];
    $signer = Signer::getSigner($sign_plugin);

    if(!$signer) {
      throw new Exception('Cant find plugin ' . $sign_plugin);
    }

    $result = $signer->prepareSignData($uri);
  }
  catch (SignerException $e) {
    drupal_set_message(t($e->getMessage()), 'error');
    watchdog(
      'signer_api',
      $e->getSystemMessage(),
      array(),
      WATCHDOG_ERROR
    );
    return FALSE;
  }
  catch(Exception $e) {
    drupal_set_message(t('Cant sign document'), 'error');
    watchdog(
      'signer_api',
      'Cant prepare data to sign. Detail @detail',
      array('@detail' => $e->getMessage()),
      WATCHDOG_ERROR
    );
  }

  drupal_json_output($result);
  module_invoke_all('exit');
  exit();
}

function signer_api_store_sign() {
  global $user;
  $code = FALSE;
  $account = user_load($user->uid);

  if(!empty($_SESSION['masquerading'])) {
    $mandatar = user_load($_SESSION['masquerading']);
    if(isset($mandatar->roles[MA_ROLE])) {
      $account = $mandatar;
    }
  }

  try {
    $sign_plugin = (isset($_POST['param']['sign_plugin']) ? $_POST['param']['sign_plugin'] : NULL);
    $idnp = isset($account->field_idnp[LANGUAGE_NONE][0]['value']) ? $account->field_idnp[LANGUAGE_NONE][0]['value'] : '';

    $signer = Signer::getSigner($sign_plugin, array(
      'fid' => (isset($_POST['param']['fid']) ? $_POST['param']['fid'] : NULL),
      'module' => (isset($_POST['param']['module']) ? $_POST['param']['module'] : NULL),
      'hash' => (isset($_POST['param']['hash']) ? $_POST['param']['hash'] : NULL),
      'rsa_sign' => (isset($_POST['param']['document_signed']) ? $_POST['param']['document_signed'] : NULL),
      'uid' => $account->uid,
      'idnp' => $idnp,
      'operation' => Signer::OPERATION_ADD,
      'sign_type' => $sign_plugin,
      'additional_data' => (isset($_POST['param']['additional_data']) ? $_POST['param']['additional_data'] : array())
    ));

    if(!$signer) {
      throw new SignerException('Cant sign document', 'Cant find plugin ' . $sign_plugin);
    }

    $signer->validate();
    $signer->save();
    $code = TRUE;
    drupal_set_message(t('Document was signed successfully'), 'status', FALSE);
  }
  catch (SignerException $e) {
    drupal_set_message(t($e->getMessage()), 'error');
    watchdog(
      'signer_api',
      $e->getSystemMessage(),
      array(),
      WATCHDOG_ERROR);
  }
  catch(Exception $e) {
    drupal_set_message(t('Cant sign document'), 'error');
    watchdog(
      'signer_api',
      'Cant validate data or save data. Detail @detail',
      array('@detail' => $e->getMessage()),
      WATCHDOG_ERROR
    );
  }

  $status = array(
    'code' => $code,
  );

  drupal_json_output($status);
  module_invoke_all('exit');
  exit();
}

function signer_api_unsign() {
  global $user;
  $code = FALSE;

  $users = module_invoke_all('signer_sign_user');
  if(count($users)) {
    $account = end($users);
  }
  else {
    $account = user_load($user->uid);
  }

  try {
    $sign_plugin = (isset($_POST['param']['sign_plugin']) ? $_POST['param']['sign_plugin'] : NULL);
    $idnp = isset($account->field_idnp[LANGUAGE_NONE][0]['value']) ? $account->field_idnp[LANGUAGE_NONE][0]['value'] : '';

    $signer = Signer::getSigner($sign_plugin, array(
      'fid' => (isset($_POST['param']['fid']) ? $_POST['param']['fid'] : NULL),
      'module' => (isset($_POST['param']['module']) ? $_POST['param']['module'] : NULL),
      'hash' => (isset($_POST['param']['hash']) ? $_POST['param']['hash'] : NULL),
      'rsa_sign' => (isset($_POST['param']['document_signed']) ? $_POST['param']['document_signed'] : NULL),
      'uid' => $account->uid,
      'idnp' => $idnp,
      'sign_type' => $sign_plugin,
      'operation' => Signer::OPERATION_REMOVE,
      'additional_data' => (isset($_POST['param']['additional_data']) ? $_POST['param']['additional_data'] : array()),
    ));
    if(!$signer) {
      throw new SignerException('Cant unsign document', 'Cant find plugin ' . $sign_plugin);
    }

    $signer->validate();
    $signer->unsign();
    $code = TRUE;
    drupal_set_message(t('Document was unsigned successfully'), 'status', FALSE);
  }
  catch (SignerException $e) {
    drupal_set_message(t($e->getMessage()), 'error');
    watchdog(
      'signer_api',
      $e->getSystemMessage(),
      array(),
      WATCHDOG_ERROR);
  }
  catch(Exception $e) {
    drupal_set_message(t('Cant unsign document'), 'error');
    watchdog(
      'signer_api',
      'Cant validate data or save data. Detail @detail',
      array('@detail' => $e->getMessage()),
      WATCHDOG_ERROR
    );
  }

  $status = array(
    'code' => $code,
  );

  drupal_json_output($status);
  module_invoke_all('exit');
  exit();
}
