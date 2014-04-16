var Signer = {
	'initialized': false,
	'self': null
};

function  initApplet(app) {
}
function StartSign() {
  Signer.self.sign();
}
function signOk() {
  Signer.self.approve();
  window.location.reload(true);
  sign_process = false;

}
function signFailed() {
  alert(Drupal.t('Signature failed'));
  sign_process = false;
}

function receive(signs, datas) {
    var param = {
      'document_signed': signs[0],
      'hash': Drupal.settings.sign_data_hash['hash'],
      'fid': Drupal.settings.sign_data_file['fid'],
      'additional_data': {
        'document_id': Drupal.settings.sign_doc_id,
        'module': Drupal.settings.sign_module,
        'mode': Drupal.settings.sign_service.sign_type
      }
    }
    saveSign(param);
}
Signer.initialize = function( dataToSign ) {
  if(!Signer.initialized) {
    if(!navigator.javaEnabled()) {
      alert(Drupal.t('JRE was not found. Please install Java virtual machine'));
      return false;
    }
    var applet = document.createElement('applet');
    applet.setAttribute('archive', Drupal.settings.signer.path);
    applet.setAttribute('code', Drupal.settings.signer.namespace);
    applet.setAttribute('id', 'SignApplet');
    applet.setAttribute('style', 'width:0; height: 0');
    applet.setAttribute('mayscript', true);

    var param = document.createElement('param');
    param.setAttribute('name', 'sign.format');
    param.setAttribute('value', 'XADES');
    applet.appendChild(param);

    var param = document.createElement('param');
    param.setAttribute('name', 'sign.qualifiedForced');
    param.setAttribute('value', 'true');
    applet.appendChild(param);

    var param = document.createElement('param');
    param.setAttribute('name', 'sign.qualifiedForcedChangeAllowed');
    param.setAttribute('value', 'true');
    applet.appendChild(param);

    var param = document.createElement('param');
    param.setAttribute('name', 'sign.pkcs11.useLocalDrivers');
    param.setAttribute('value', 'true');
    applet.appendChild(param);

    var param = document.createElement('param');
    param.setAttribute('name', 'sign.pkcs11.drivers');
    param.setAttribute('value', 'cryptoCertumPKCS11;acospkcs11');
    applet.appendChild(param);

    var param = document.createElement('param');
    param.setAttribute('name', 'sign.dataType');
    param.setAttribute('value', 'SITE');
    applet.appendChild(param);

    var param = document.createElement('param');
    param.setAttribute('name', 'sign.data');
    param.setAttribute('value', dataToSign);
    applet.appendChild(param);

    var param = document.createElement('param');
    param.setAttribute('name', 'sign.data.contentType');
    param.setAttribute('value', 'application/octet-stream');
    applet.appendChild(param);

    var param = document.createElement('param');
    param.setAttribute('name', 'sign.dataFileName');
    param.setAttribute('value', 'doc.xml');
    applet.appendChild(param);

    var param = document.createElement('param');
    param.setAttribute('name', 'gui.visible');
    param.setAttribute('value', 'false');
    applet.appendChild(param);

    var param = document.createElement('param');
    param.setAttribute('name', 'gui.dialogStyle');
    param.setAttribute('value', 'embed');
    applet.appendChild(param);

    var param = document.createElement('param');
    param.setAttribute('name', 'gui.locale');
    param.setAttribute('value', 'mo_MD');
    applet.appendChild(param);

    var param = document.createElement('param');
    param.setAttribute('name', 'callback.class');
    param.setAttribute('value', 'pl.unizeto.procertum.applet.callback.JavaScriptCallback');
    applet.appendChild(param);

    var param = document.createElement('param');
    param.setAttribute('name', 'callback.function');
    param.setAttribute('value', 'receive');
    applet.appendChild(param);

    var param = document.createElement('param');
    param.setAttribute('name', 'callback.base64');
    param.setAttribute('value', 'true');
    applet.appendChild(param);

    var param = document.createElement('param');
    param.setAttribute('name', 'callback.signaturesAsStrings');
    param.setAttribute('value', 'true');
    applet.appendChild(param);

    var param = document.createElement('param');
    param.setAttribute('name', 'callback.javascript.afterSuccessfulSigning');
    param.setAttribute('value', 'signOk');
    applet.appendChild(param);

    var param = document.createElement('param');
    param.setAttribute('name', 'callback.javascript.afterFailedSigning');
    param.setAttribute('value', 'signFailed');
    applet.appendChild(param);

    var param = document.createElement('param');
    param.setAttribute('name', 'gui.initApplet');
    param.setAttribute('value', 'initApplet');
    applet.appendChild(param);

    var param = document.createElement('param');
    param.setAttribute('name', 'sign.sign.autoApproveAfterSign');
    param.setAttribute('value', 'true');
    applet.appendChild(param);

    if (Signer.initialized == true) {
      var element = document.getElementById("SignApplet");
      element.parentNode.removeChild(element);
    }

    Signer.self = document.body.appendChild(applet);
    Signer.initialized = true;
  }
}

function stcSignDocument() {
  if(sign_process == false) {
    sign_process = true;
    var file_data  = getDocumentData();
    if (file_data.length == 0) {
      alert(Drupal.t('Signing failed!'));
      sign_process = false;
      return;
    }
    var hash = getHash(file_data);

    if (hash == false) {
      alert(Drupal.t('Signing failed!'));
      sign_process = false;
      return;
    }

    Signer.initialize(hash['base64']);
    Drupal.settings.sign_data_file = file_data;
    Drupal.settings.sign_data_hash = hash;
    StartSign();

  }
  sign_process = false;
}
