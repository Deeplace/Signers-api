Drupal.behaviors.msign = {
  attach: function(context, settings) {
    var msign_form = jQuery('#msign_form', context);

    if(msign_form.length > 0) {
      msign_form.dialog({
        autoOpen: false,
        show: "blind",
        hide: "explode",
        minWidth: 400
      });
    }

    jQuery('.dialog-close').live('click', function() {
      window.location.reload(true);
      return false;
    });
    jQuery('.msign_document', context).bind('click', function(e) {
      e.preventDefault();
      msign_form.dialog("open");
    });


    jQuery('#msign_form #msign_form_submit', context).bind('click', function(e) {
      e.preventDefault();
      sign_process = true;
      MsignDocument();
    });
    jQuery('#msign_form #msign_form_return', context).bind('click', function(e) {
      e.preventDefault();
      msign_form.dialog('close');
      jQuery('#signer-dialog', context).dialog('open');
    });


    function MsignDocument() {
      var file_data = getDocumentData();
      if (file_data.length == 0 || typeof file_data == 'undefined') {
        alert(Drupal.t('Signing failed!'));
        msign_form.dialog('close');
        sign_process = false;
        return;
      }

      var hash = getHash(file_data);
      if(hash == false || hash.length == 0) {
        alert(Drupal.t('Signing failed!'));
        msign_form.dialog('close');
        sign_process = false;
        return ;
      }

      var result = {};
      var param = {
        'hash': hash,
        'fid': file_data['fid'],
        'operation' : Drupal.settings.sign_operation,
        'sign_plugin' : Drupal.settings.sign_service.sign_plugin,
        'module':Drupal.settings.sign_module,
        'additional_data': {
          'mode': Drupal.settings.sign_mode,
          'document_id': Drupal.settings.sign_doc_id
        }
      }

      jQuery.ajax({
        'async': false,
        'type': 'POST',
        'data': {'param' : param},
        'success': function(obj) {
          if(obj.code == true) {
            var action = jQuery('#signer-msign-redirect-form').attr('action');
            action += '/' + obj.requestId;
            jQuery('#signer-msign-redirect-form').attr('action', action);
            jQuery('#signer-msign-redirect-form').submit();
          }
          else {
            alert(Drupal.t('Signing failed!'));
            msign_form.dialog('close');
            sign_process = false;
            return;
          }
        },
        'url': Drupal.settings.msign.sign_url,
        'dataType': 'json'
      });
    }
  }
}
