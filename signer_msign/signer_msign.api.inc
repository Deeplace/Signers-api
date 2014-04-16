<?php

function signer_msign_form() {
  $form = array();
  $form['content'] = array(
    '#prefix' => '<div id="msign_form">',
    '#suffix' => '</div>'
  );

  $def_value = variable_get('signer_msign_front_description', array());
  $form['content']['description'] = array(
    '#markup' => (isset($def_value['value']) ? $def_value['value'] : ''),
  );

  $form['content']['redirect_form'] = drupal_get_form('signer_msign_redirect_form');

  $form['content']['submit'] = array(
    '#markup' => l(t('Continue'), '#', array('external' => TRUE, 'absolute' => TRUE, 'attributes' => array('id' => 'msign_form_submit'))),
  );
  $form['content']['back'] = array(
    '#markup' => l(t('Back'), '#', array('external' => TRUE, 'absolute' => TRUE, 'attributes' => array('id' => 'msign_form_return', 'class' => 'right-link'))),
  );

  return $form;
}

function signer_msign_redirect_form($form, $form_state) {
  $form['#action'] = variable_get('signer_msign_msign_redirect_url', '');

  $form['ReturnUrl'] = array(
    '#type' => 'hidden',
    '#value' => url('msign/response', array('absolute' => TRUE))
  );

  return $form;
}
