<?php

/**
 * Implements hook_variable_info().
 */

function signer_msign_variable_info($options) {
  $variable['signer_msign_front_description'] = array(
    'type' => 'string',
    'title' => t('signer msign front description', array(), $options),
    'default' => '',
    'required' => TRUE,
    'group' => 'mobile_sign',
  );

  return $variable;
}

/**
 * Implements hook_variable_group_info().
 */
function signer_msign_variable_group_info() {
  $groups['signer_msign'] = array(
    'title' => t('Msign'),
    'access' => 'administer site configuration',
    'path' => array('admin/config/services/msign'),
  );
  return $groups;
}
