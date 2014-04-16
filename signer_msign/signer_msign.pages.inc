<?php

function signer_msign_sign_doc() {
  $code = FALSE;
  global $user;

  $users = module_invoke_all('signer_sign_user');
  if(count($users)) {
    $account = end($users);
  }
  else {
    $account = user_load($user->uid);
  }

  $status = array(
    'code' => $code,
  );

  try {
    $sign = new MSign(
      array(
        'fid' => (isset($_POST['param']['fid']) ? $_POST['param']['fid'] : NULL),
        'module' => (isset($_POST['param']['module']) ? $_POST['param']['module'] : NULL),
        'hash' => isset($_POST['param']['hash']) ? $_POST['param']['hash'] : NULL,
        'uid' => $account->uid,
        'operation' => ((isset($_POST['param']['operation']) && $_POST['param']['operation'] == 'sign') ? Signer::OPERATION_ADD : Signer::OPERATION_REMOVE),
        'sign_type' => (isset($_POST['param']['sign_plugin']) ? $_POST['param']['sign_plugin'] : NULL),
        'additional_data' => (isset($_POST['param']['additional_data']) ? $_POST['param']['additional_data'] : array()),
      )
    );

    $status = $sign->sign();
    $status['code'] = TRUE;
  }
  catch (SignerException $e) {
    watchdog(
      'signer_msign',
      $e->getSystemMessage(),
      array(),
      WATCHDOG_ERROR);
  }
  catch(Exception $e) {
    watchdog(
      'signer_msign',
      'Cant sing data. Detail @detail',
      array('@detail' => $e->getMessage()),
      WATCHDOG_ERROR
    );
  }

  drupal_json_output($status);
  module_invoke_all('exit');
  exit();
}

function signer_msign_response($request_id = NULL) {
  $build = array();
  if(isset($request_id)) {
    $_POST['RequestID'] = $request_id;
  }

  if(empty($_POST['RequestID'])) {

    $build['content'] = array(
      '#markup' => t('Internal error. Please try again')
    );

    return $build;
  }

  $transaction = db_select('msign_transaction', 'mt')
    ->fields('mt')
    ->condition('mt.request_id', $_POST['RequestID'])
    ->execute()->fetchObject();

  if(!$transaction) {
    $build['content'] = array(
      '#markup' => t('Internal error. Please try again')
    );

    return $build;
  }

  try {
    $info = unserialize($transaction->info);
    $sign_user = user_load($info['uid']);
    $idnp = isset($sign_user->field_idnp[LANGUAGE_NONE][0]['value']) ? $sign_user->field_idnp[LANGUAGE_NONE][0]['value'] : '';
    $signer = new MSign(
      array(
        'fid' => (isset($info['fid']) ? $info['fid'] : NULL),
        'module' => (isset($info['module']) ? $info['module'] : NULL),
        'hash' => (isset($info['hash']) ? $info['hash'] : NULL),
        'uid' => (isset($info['uid']) ? $info['uid'] : NULL),
        'idnp' => $idnp,
        'operation' => (isset($info['operation']) ? $info['operation'] : Signer::OPERATION_ADD),
        'sign_type' => (isset($info['sign_type']) ? $info['sign_type'] : NULL),
        'additional_data' => (isset($info['additional_data']) ? $info['additional_data'] : array()),
      )
    );

    $data = $signer->loadRequestData($_POST['RequestID']);

    if($data->Status == 'Pending') {
      $build['content'] = array(
        '#markup' => t('Signing of document is in process.')
      );
    }
    elseif($data->Status == 'Failure') {
      $build['content'] = array(
        '#markup' => t('Signature invalid.')
      );
    }
    elseif($data->Status == 'Success') {
      if($signer->operation == Signer::OPERATION_ADD) {
        if($signer->checkCertificate()) {
          $signer->save();
          $build['content'] = array(
            '#markup' => t('Document is successfully signed.')
          );
        }
        else {
          $build['content'] = array(
            '#markup' => t('Signer of document has different IDNP specified in profile')
          );
        }
      }
      else {
        $signer->unsign();
        $build['content'] = array(
          '#markup' => t('Document is successfully unsigned.')
        );
      }
    }
    else {
      $build['content'] = array(
        '#markup' => t('Unknown error. Please try again')
      );
    }

    $transaction->status = $data->Status;
    $transaction->updated = time();
    drupal_write_record('msign_transaction', $transaction, 'request_id');
  }
  catch (SignerException $e) {
    watchdog(
      'msign',
      $e->getSystemMessage(),
      array(),
      WATCHDOG_ERROR);
    $build['content'] = array(
      '#markup' => t($e->getMessage())
    );
  }
  catch(Exception $e) {
    watchdog(
      'msign',
      'Cant validate data or save data. Detail @detail',
      array('@detail' => $e->getMessage()),
      WATCHDOG_ERROR
    );

    $build['content'] = array(
      '#markup' => t($e->getMessage())
    );
  }

  return $build;
}
