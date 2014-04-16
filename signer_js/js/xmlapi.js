
function signAsyncDocument() {
  var file_data  = getDocumentData();
  if (file_data.length == 0) {
    if(Drupal.settings.sign_operation == 'sing') {
      alert(Drupal.t('Signing failed!'));
    }
    else {
      alert(Drupal.t('Unsigning failed!'));
    }

    sign_process = false;
    return;
  }
  var hash = getHash(file_data);

  result = false;
  try {
    var signed_data = SignAsyncData(private_key, hash, Drupal.settings.sign_doc_id, file_data['fid']);
  } catch(e) {
    reload_page = false;
  }

  return result;
}

function RSAsingResult(hexSign, doc_id, s, fid) {
  var param = {
    'document_signed': hexSign,
    'hash': s,
    'fid': fid,
    'additional_data': {
      'document_id': doc_id
    }
  }

  saveSign(param);

  sign_process = false;
  window.location.reload(true);
}

function unsignDocument(id, file_id, module) {
  var result = null;
  var req = jQuery.ajax({
    'data': {'document_id': id, 'file_id': file_id, 'module': module},
    'async': false,
    'type': 'POST',
    'url': '/signer_js/delete_sign',
    'dataType': 'json',
    'success': function(obj) {
      result = obj;
    }
  });
  return result;
}
