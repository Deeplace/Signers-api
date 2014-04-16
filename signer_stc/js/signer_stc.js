Drupal.behaviors.signer_stc = {
  attach: function(context) {
    jQuery('.stc_sign_document').click(function() {
      stcSignDocument();
    });
  }
}
