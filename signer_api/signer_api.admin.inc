<?php

function signer_api_settings_form($form, &$form_state) {
  $form['signer_api_dialog_pages'] = array(
    '#type' => 'textarea',
    '#title' => t('Pages with sign functional'),
    '#default_value' => variable_get('signer_api_dialog_pages', '')
  );

  return system_settings_form($form);
}
