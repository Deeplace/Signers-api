<?php

/**
 * Implements hook_variable_info().
 */

function mobile_sign_variable_info($options) {
  $variable['mobile_sign_phone_display_message'] = array(
    'type' => 'string',
    'title' => t('Phone display message', array(), $options),
    'default' => '',
    'required' => TRUE,
    'group' => 'mobile_sign',
  );

  $variable['mobile_sign_user_display_message'] = array(
    'type' => 'string',
    'title' => t('User display message', array(), $options),
    'default' => '',
    'required' => TRUE,
    'group' => 'mobile_sign',
  );

  return $variable;
}

/**
 * Implements hook_variable_group_info().
 */
function mobile_sign_variable_group_info() {
  $groups['mobile_sign'] = array(
    'title' => t('Mobile sign'),
    'access' => 'administer site configuration',
    'path' => array('admin/config/services/mobile_sign'),
  );
  return $groups;
}
