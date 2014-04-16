<?php

function signer_msign_settings_form($form, $form_state) {
  $form['signer_msign_msign_redirect_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Msign user redirect url'),
    '#default_value' => variable_get('signer_msign_msign_redirect_url', '')
  );

  $form['signer_msign_wsdl_path'] = array(
    '#type' => 'textfield',
    '#title' => t('Path to wsdl'),
    '#default_value' => variable_get('signer_msign_wsdl_path', '')
  );

  $form['signer_msign_authentication_certificate'] = array(
    '#type' => 'textfield',
    '#title' => t('Path to authentication certificate'),
    '#default_value' => variable_get('signer_msign_authentication_certificate', '')
  );

  $form['signer_msign_remove_sign_request_after'] = array(
    '#type' => 'textfield',
    '#title' => t('Remove sign request after n days'),
    '#default_value' => variable_get('signer_msign_remove_sign_request_after', '15'),
    '#element_validate' => array('element_validate_number')
  );

  $def_value = variable_get('signer_msign_front_description', array());
  $form['signer_msign_front_description'] = array(
    '#type'=>'text_format',
    '#title' => t('Description on modal window when choose mpay'),
    '#format'=>'full_html',
    '#default_value' => (isset($def_value['value']) ? $def_value['value'] : '')
  );

  return system_settings_form($form);
}

function signer_msign_settings_form_validate($form, $form_state) {
  if(!empty($form_state['values']['signer_msign_authentication_certificate'])) {
    if(!file_exists($form_state['values']['signer_msign_authentication_certificate'])) {
      form_set_error('signer_msign_authentication_certificate', t('Authentication certificate file not found'));
    }

    if(!is_readable($form_state['values']['signer_msign_authentication_certificate'])) {
      form_set_error('signer_msign_authentication_certificate', t('Authentication certificate is not readable'));
    }
  }
}
