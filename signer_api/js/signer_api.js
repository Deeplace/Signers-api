var sign_process = false;
Drupal.behaviors.signer_api = {
  attach: function(context) {
    jQuery('#signer-dialog', context).dialog({
          autoOpen: false,
          show: "blind",
          hide: "explode",
          minWidth: 450
        });
    jQuery('.signer_link', context).bind('click',function() {
      Drupal.settings.sign_operation = 'sign';
      Drupal.settings.sign_data_callback = jQuery(this).attr('data_callback');
      Drupal.settings.sign_doc_id = jQuery(this).attr('doc_id');
      Drupal.settings.sign_module = jQuery(this).attr('module');
      Drupal.settings.sign_mode = jQuery(this).attr('mode');
      jQuery('#signer-dialog').dialog('option', 'title', Drupal.settings.signer_api.sign_text);
      jQuery('#signer-dialog').dialog('open');

      return false;
    });
    jQuery('#signer-dialog a').click(function() {
      if (typeof Drupal.settings.sign_service == 'undefined') {
        Drupal.settings.sign_service = {};
      }
      Drupal.settings.sign_service.sign_plugin = jQuery(this).attr('signer_plugin');
      jQuery('#signer-dialog').dialog('close');
    });

    jQuery('.unsign_link', context).bind('click',function() {
      Drupal.settings.sign_operation = 'unsign';
      Drupal.settings.sign_data_callback = jQuery(this).attr('data_callback');
      Drupal.settings.sign_doc_id = jQuery(this).attr('doc_id');
      Drupal.settings.sign_module = jQuery(this).attr('module');
      jQuery('#signer-dialog').dialog('option', 'title', Drupal.settings.signer_api.unsign_text);
      jQuery('#signer-dialog').dialog('open');

      return false;
    });
  }
}

function getDocumentData() {
  var result = [];
  var req = jQuery.ajax({
    'async': false,
    'type': 'GET',
    'success': function(obj) {
      result = obj;
    },
    'url': Drupal.settings.sign_data_callback,
    'dataType': 'json'
  });

  return result;
}

function getHash(file_data) {
  var result = {};
  var req = jQuery.ajax({
    'data': {'fid': file_data['fid'], 'uri': file_data['uri'], 'sign_plugin': Drupal.settings.sign_service.sign_plugin},
    'async': false,
    'type': 'POST',
    'success': function(obj) {
      result = obj == false ? obj : obj['hash'];
    },
    'url': '/signer_api/prepare_data_to_sign',
    'dataType': 'json'
  });
  return result;
}

function saveSign(param) {
  param.sign_plugin = Drupal.settings.sign_service.sign_plugin;
  param.module = Drupal.settings.sign_module;
  param.additional_data.mode = Drupal.settings.sign_mode;

  var req = jQuery.ajax({
    'data': {'param': param},
    'async': false,
    'type': 'POST',
    'url': (Drupal.settings.sign_operation == 'sign' ? Drupal.settings.signer_api.sign_store_url  : Drupal.settings.signer_api.unsign_store_url),
    'dataType': 'json',
    'success': function(obj) {
      result = obj;
    }
  });
}
