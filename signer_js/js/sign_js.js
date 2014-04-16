Drupal.behaviors.signer_js = {
  attach: function(context) {
    if(jQuery('#signer_form', context).length > 0) {
      jQuery('#signer_form', context).dialog({
        autoOpen: false,
        show: "blind",
        hide: "explode",
        minWidth: 400,
        beforeClose: function(event, ui) {
          if(sign_process === true) {
            alert(Drupal.settings.sign_service.sign_process_string);
            return false;
          }
          return true;
        }
      });
    }

  jQuery('a.sign_document', context).click(function() {
    document_current_object = jQuery(this);
    if(Drupal.settings.sign_operation == 'sign') {
      jQuery("#signer_form #sign_link").text(Drupal.t('Sign document'));
    }
    else {
      jQuery("#signer_form #sign_link").text(Drupal.t('Unsign document'));
    }

    jQuery("#signer_form", context).dialog("open");

    return false;
  });

  jQuery('#sign_link', context).bind('click', function(){
    if(sign_process === false) {
      if(private_key.length > 0) {
        sign_process = true;
        signAsyncDocument();
      }
      else {
        alert(Drupal.settings.sign_service.not_found_key_string);
      }
    }
    return false;
  });

  jQuery('#back_link', context).bind('click', function(){
      jQuery('#signer_form', context).dialog('close');
      jQuery('#signer-dialog', context).dialog('open');
      return false;
  });



  jQuery('.unsign_document').unbind('click');
  jQuery('.unsign_document').bind('click', function(){
    if (confirm(Drupal.settings.sign_service.unsign_confirm_string)) {
      result = unsignDocument(jQuery(this).attr('doc_id'), jQuery(this).attr('file_id'), jQuery(this).attr('module'));
      window.location.reload(true);
    }
    return false;
  });
  }
};




