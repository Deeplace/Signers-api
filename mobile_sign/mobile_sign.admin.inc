<?php

function mobile_sign_settings_form($form, $form_state) {
  $module_path = drupal_get_path('module', 'mobile_sign');

  $form['mobile_sign_wsdl_path'] = array(
    '#type' => 'textfield',
    '#title' => t('Path to wsdl'),
    '#default_value' => variable_get('mobile_sign_wsdl_path', $module_path . '/wsdl/sign.wsdl')
  );

  $form['mobile_verify_wsdl_path'] = array(
    '#type' => 'textfield',
    '#title' => t('Path to wsdl'),
    '#default_value' => variable_get('mobile_verify_wsdl_path', $module_path . '/wsdl/verify.wsdl')
  );

  $form['mobile_sign_service_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Service Id'),
    '#default_value' => variable_get('mobile_sign_service_id', '')
  );

  $form['mobile_sign_remove_sign_request_after'] = array(
    '#type' => 'textfield',
    '#title' => t('Remove sign request after n days'),
    '#default_value' => variable_get('mobile_sign_remove_sign_request_after', '15'),
    '#element_validate' => array('element_validate_number')
  );

  $form['mobile_sign_phone_display_message'] = array(
    '#type' => 'textfield',
    '#title' => t('Phone display message'),
    '#default_value' => variable_get('mobile_sign_phone_display_message', 'Verification code !code'),
    '#description' => t('Available variables !code')
  );

  $form['mobile_sign_user_display_message'] = array(
    '#type' => 'textfield',
    '#title' => t('Message what user see on the site in when signing'),
    '#default_value' => variable_get('mobile_sign_user_display_message', 'Verification code !code'),
    '#description' => t('Available variables !code')
  );


  return system_settings_form($form);
}
