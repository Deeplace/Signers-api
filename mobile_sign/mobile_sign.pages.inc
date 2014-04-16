<?php

function mobile_sign_doc() {
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
    $sign = new MobileSign(
      array(
        'fid' => (isset($_POST['param']['fid']) ? $_POST['param']['fid'] : NULL),
        'module' => (isset($_POST['param']['module']) ? $_POST['param']['module'] : NULL),
        'hash' => isset($_POST['param']['hash']) ? $_POST['param']['hash'] : NULL,
        'uid' => $account->uid,
        'operation' => ((isset($_POST['param']['operation']) && $_POST['param']['operation'] == 'sign') ? Signer::OPERATION_ADD : Signer::OPERATION_REMOVE),
        'sign_type' => (isset($_POST['param']['sign_plugin']) ? $_POST['param']['sign_plugin'] : NULL),
        'additional_data' => (isset($_POST['param']['additional_data']) ? $_POST['param']['additional_data'] : array()),
        'phone' => isset($_POST['param']['phone']) ? $_POST['param']['phone'] : NULL,
      )
    );

    $status = $sign->sign();
    $status['code'] = TRUE;
  }
  catch (SignerException $e) {
    watchdog(
      'mobile_sign',
      $e->getSystemMessage(),
      array(),
      WATCHDOG_ERROR);
  }
  catch(Exception $e) {
    watchdog(
      'mobile_sign',
      'Cant sing data. Detail @detail',
      array('@detail' => $e->getMessage()),
      WATCHDOG_ERROR
    );
  }

  drupal_json_output($status);
  module_invoke_all('exit');
  exit();
}

function mobile_sign_response() {
  $received_data = file_get_contents('php://input');
  watchdog('mobile_sign_receive_data', 'Receive data: !data', array('!data' => $received_data), WATCHDOG_INFO);

  $response = MobileSign::SIGN_RESPONSE_XML_TEMPLATE;
  drupal_add_http_header('Content-Type', 'text/xml');
  libxml_use_internal_errors(true);

  try {
    $xml = new SimpleXMLElement($received_data);
    $transaction_id = (string)$xml->children('ns1', true)->TransactionId;
    $status_code = (string)$xml->children('ns1', true)->StatusCode;
    $signature_result = (string)$xml->children('ns1', true)->SignatureResult;

    $transaction = db_select('mobile_id_transaction', 'mt')
      ->fields('mt')
      ->condition('mt.transactionId', $transaction_id)
      ->condition('mt.status', MobileSign::OUSTANDING_TRANSACTION)
      ->execute()->fetchObject();

    if(!$transaction) {
      drupal_add_http_header('Status', '404 Not Found');
      echo strtr($response, array(':statusCode' => MobileSign::INTERNAL_ERROR, ':statusMessage' => 'InternalERROR'));
      exit();
    }

    try {
      if($status_code == MobileSign::VALID_SIGNATURE) {
        $info = unserialize($transaction->info);
        $sign_user = user_load($info['uid']);
        $idnp = isset($sign_user->field_idnp[LANGUAGE_NONE][0]['value']) ? $sign_user->field_idnp[LANGUAGE_NONE][0]['value'] : '';
        $signer = new MobileSign(
          array(
            'fid' => (isset($info['fid']) ? $info['fid'] : NULL),
            'module' => (isset($info['module']) ? $info['module'] : NULL),
            'hash' => (isset($info['hash']) ? $info['hash'] : NULL),
            'uid' => (isset($info['uid']) ? $info['uid'] : NULL),
            'idnp' => $idnp,
            'operation' => (isset($info['operation']) ? $info['operation'] : Signer::OPERATION_ADD),
            'sign_type' => (isset($info['sign_type']) ? $info['sign_type'] : NULL),
            'additional_data' => (isset($info['additional_data']) ? $info['additional_data'] : array()),
            'signatureData' => base64_decode($signature_result),
          )
        );

        if($signer->operation == Signer::OPERATION_ADD) {
          if($signer->checkCertificate()) {
            $signer->save();
            $transaction->status = $status_code;
            $transaction->updated = time();
            $transaction->user_message = 'Document was signed successfully';
          }
          else {
            $transaction->status = MobileSign::WRONG_IDNP;
            $transaction->updated = time();
            $transaction->user_message = 'IDNP specified in profile and IDNP specified in certificate are different';
          }
          drupal_write_record('mobile_id_transaction', $transaction, 'transactionId');
          echo strtr($response, array(':statusCode' => MobileSign::RESPONSE_REQUEST_OK, ':statusMessage' => 'Requested data have processed successfully.'));
          exit();
        }
        else {
          $signer->unsign();
          $transaction->user_message = 'Document was unsigned successfully';
          $transaction->status = MobileSign::VALID_SIGNATURE;
          drupal_write_record('mobile_id_transaction', $transaction, 'transactionId');
          echo strtr($response, array(':statusCode' => MobileSign::RESPONSE_REQUEST_OK, ':statusMessage' => 'Requested data have processed successfully.'));
          exit();
        }
      }
    }
    catch (SignerException $e) {
      watchdog(
        'mobile_sign',
        $e->getSystemMessage(),
        array(),
        WATCHDOG_ERROR);
      $transaction->user_message = $e->getMessage();
      $transaction->status = MobileSign::INTERNAL_ERROR;
      drupal_write_record('mobile_id_transaction', $transaction, 'transactionId');
    }
    catch(Exception $e) {
      watchdog(
        'mobile_sign',
        'Cant validate data or save data. Detail @detail',
        array('@detail' => $e->getMessage()),
        WATCHDOG_ERROR
      );
      $transaction->user_message = $e->getMessage();
      $transaction->status = MobileSign::INTERNAL_ERROR;
      drupal_write_record('mobile_id_transaction', $transaction, 'transactionId');
    }
  }
  catch(Exception $e) {
    watchdog('mobile_sign', 'Receive invalid xml data !data', array('!data' => $received_data));
  }
  echo strtr($response, array(':statusCode' => MobileSign::INTERNAL_ERROR, ':statusMessage' => 'InternalERROR'));
  exit();
}

function mobile_sign_check_status() {
  $code = FALSE;

  $status = array(
    'code' => $code,
  );

  if(isset($_POST['transactionId'])) {
    $mobile_sign_req = db_select('mobile_id_transaction', 'mt')
      ->fields('mt')
      ->condition('mt.transactionId', $_POST['transactionId'])
      ->execute()->fetchObject();

    if($mobile_sign_req && !in_array($mobile_sign_req->status, array(MobileSign::OUSTANDING_TRANSACTION))) {
      $status = array(
        'code' => TRUE,
        'user_message' => t($mobile_sign_req->user_message)
      );
    }
  }

  drupal_json_output($status);
  module_invoke_all('exit');
  exit();
}
