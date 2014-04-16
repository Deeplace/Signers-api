Drupal.behaviors.mobile_sign = {
  attach: function(context, settings) {
    var mobile_sign_form = jQuery('#mobile_sign_form', context);

    if(mobile_sign_form.length > 0) {
      mobile_sign_form.dialog({
        autoOpen: false,
        show: "blind",
        hide: "explode",
        minWidth: 400,
        beforeClose: function(event, ui) {
          /*
          if(sign_process == true) {
            alert(Drupal.settings.sign_service.sign_process_string);
            return false;
          }
          return true;
          */
        }
      });
    }

    jQuery('.dialog-close').live('click', function() {
      window.location.reload(true);
      return false;
    });
    jQuery('.mobile_sign_document', context).bind('click', function(e) {
      e.preventDefault();
      mobile_sign_form.dialog("open");
    });


    jQuery('#mobile_sign_form #mobile_sign_form_submit', context).bind('click', function(e) {
      e.preventDefault();
      sign_process = true;
      var phone = '+373' + jQuery('#mobile_sign_form .mobile_phone').val();
      if (phone.length != 12) {
        alert(Drupal.t('Invalid number.'));
        return;
      }
      MobilesignDocument(phone);
    });

    jQuery('#mobile_sign_form #mobile_sign_form_return', context).bind('click', function(e) {
      e.preventDefault();
      mobile_sign_form.dialog('close');
      jQuery('#signer-dialog', context).dialog('open');
    });

    jQuery('#mobile_sign_form .mobile_phone').on('input', function() {
      var _this = jQuery(this);
      var nr = _this.val();
      var newNr = '';
      var ch;
      var length = nr.length <= 8 ? nr.length : 8;
      for (var i= 0; i < length; i++) {
        ch = nr.charAt(i);
        if (ch >= '0' && ch <= '9') {
          newNr += ch;
        }
      }
      if (nr != newNr) {
        _this.val(newNr);
        nr = newNr;
      }
    });

    function MobilesignDocument(phone_number) {
      var file_data = getDocumentData();
      if (file_data.length == 0 || typeof file_data == 'undefined') {
        alert(Drupal.t('Signing failed!'));
        mobile_sign_form.dialog('close');
        sign_process = false;
        return;
      }

      var hash = getHash(file_data);
      if(hash == false || hash.length == 0) {
        alert(Drupal.t('Signing failed!'));
        mobile_sign_form.dialog('close');
        sign_process = false;
        return ;
      }

      var result = {};
      var param = {
        'hash': hash,
        'phone': phone_number,
        'fid': file_data['fid'],
        'operation' : Drupal.settings.sign_operation,
        'sign_plugin' : Drupal.settings.sign_service.sign_plugin,
        'module':Drupal.settings.sign_module,
        'additional_data': {
          'mode': Drupal.settings.sign_mode,
          'document_id': Drupal.settings.sign_doc_id
        }
      }

      jQuery('#mobile_sign_form').html(Drupal.t('Please, wait. Do not close your browser') + "<div class = 'sign-throbber'></div>");
      jQuery('.ui-dialog-titlebar button').remove();
      jQuery.ajax({
        'async': false,
        'type': 'POST',
        'data': {'param' : param},
        'success': function(obj) {
          result = obj;
        },
        'url': '/mobile_id/sign',
        'dataType': 'json'
      });

      if (result.code === true) {
        jQuery('#mobile_sign_form').html(result.successMessage + "<div class = 'sign-throbber'></div>");

        var await_result = {};
        var timeoutId = setInterval(function() {
          jQuery.ajax({
            'async': false,
            'type': 'POST',
            'data': {'transactionId': result.transactionId},
            'success': function(obj) {
              if (obj.code === true) {
                jQuery('#mobile_sign_form').html(obj.user_message + "<div><a href = '#' class = 'dialog-close'>OK</a></div>");
                clearInterval(timeoutId);
              }
            },
            'url': '/mobile_id/check_status',
            'dataType': 'json'
          });
        }, 3000);

      }
      else {
        alert(Drupal.t('Signing failed!'));
        mobile_sign_form.dialog('close');
        sign_process = false;
        return;
      }
    }


  }
}
