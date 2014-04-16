<?php

function mobile_sign_form() {
  $form = array();
  $form['content'] = array(
    '#prefix' => '<div id="mobile_sign_form">',
    '#suffix' => '</div>'
  );
  $form['content']['telephone'] = array(
    '#field_prefix' => '+373',
    '#type' => 'textfield',
    '#title' => t('Please insert your mobile phone number'),
    '#size' => 20,
    '#attributes' => array(
      'class' => array(
        'mobile_phone'
      )
    )
  );

  $form['content']['submit'] = array(
    '#markup' => l(t('Continue'), '#', array('external' => TRUE, 'absolute' => TRUE, 'attributes' => array('id' => 'mobile_sign_form_submit'))),
  );
  $form['content']['back'] = array(
    '#markup' => l(t('Back'), '#', array('external' => TRUE, 'absolute' => TRUE, 'attributes' => array('id' => 'mobile_sign_form_return', 'class' => 'right-link'))),
  );

  return $form;
}
