var reload_page = true;
var error_sign = false;
var private_key = '';

function SignAsyncData(pemkey, str, doc_id, fid) {
  var rsa = new RSAKey();
  rsa.readPrivateKeyFromPEMString(pemkey);
  var hashAlg = "sha1";
  return hSig = rsa.signAsyncString(str, hashAlg, doc_id, fid);
}

function savePrivateKey(message) {
  private_key = message;
}

jQuery(document).ready(function(){
  var flashvars = {
    button_label: Drupal.settings.sign_service.button_name,
    private_key_callback: 'savePrivateKey'
  };

  swfobject.embedSWF(
    Drupal.settings.sign_service.module_path + "/swf/getKey.swf",
    "sign-object",
    "300",
    "60",
    "10.0.0",
    Drupal.settings.sign_service.module_path + "/swf/expressInstall.swf",
    flashvars
  );
});

function _rsasign_Async_signString(s, hashAlg, doc_id, fid) {
  var hPM = _rsasign_getHexPaddedDigestInfoForString(s, this.n.bitLength(), hashAlg);
  var biPaddedMessage = parseBigInt(hPM, 16);
  this.doAsyncPrivate(biPaddedMessage, doc_id, s, fid);
}

function RSADoAsyncPrivate(x, doc_id, s, fid) {
  if(this.p == null || this.q == null)
    return x.modPow(this.d, this.n);
  setTimeout(RSADoAsyncStep1(x, this, doc_id, s, fid), 1);
}

function RSADoAsyncStep1(x, obj, doc_id, s, fid) {
  var xp = x.mod(obj.p).modPow(obj.dmp1, obj.p); // xp=cp?
  setTimeout(function(){RSADoAsyncStep2(x, obj, doc_id, s, xp, fid)}, 1);
}

function RSADoAsyncStep2(x, obj, doc_id, s, xp, fid) {
  var xq = x.mod(obj.q).modPow(obj.dmq1, obj.q); // xq=cq?
  while(xp.compareTo(xq) < 0)
    xp = xp.add(obj.p);
  // NOTE:
  // xp.subtract(xq) => cp -cq
  // xp.subtract(xq).multiply(this.coeff).mod(this.p) => (cp - cq) * u mod p = h
  // xp.subtract(xq).multiply(this.coeff).mod(this.p).multiply(this.q).add(xq) => cq + (h * q) = M
  var res = xp.subtract(xq).multiply(obj.coeff).mod(obj.p).multiply(obj.q).add(xq);
  setTimeout(function(){RSADoFinal(res, doc_id, s, fid)}, 1);
}

function RSADoFinal(res, doc_id, s, fid) {
  var hexSign = res.toString(16);
  setTimeout(function(){RSAsingResult(hexSign, doc_id, s, fid)}, 1);
}


RSAKey.prototype.doAsyncPrivate = RSADoAsyncPrivate;
RSAKey.prototype.signAsyncString = _rsasign_Async_signString;
