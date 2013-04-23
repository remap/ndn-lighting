/** 
 * @author: Wentao Shang
 * See COPYING for copyright and distribution information.
 */

var NameCryptoState = function NameCryptoState() {
	this.tv_sec = 0;  // 4 bytes
	this.tv_usec = 0; // 4 bytes
	this.seq = 0;     // 4 bytes
	this.rsvd = 0;    // 4 bytes reserved for future use
};

NameCryptoState.size = 16;

NameCryptoState.prototype.update = function() {
	var d = new Date();
	this.tv_sec = Math.floor(d / 1000.0);
	this.tv_usec = (d - this.tv_sec * 1000) * 1000;
	this.seq++;
	
	//console.log(d / 1.0);
	//console.log(this);
};

NameCryptoState.dump_int = function(value) {
	if (value < 0)
		return new Uint8Array([0xff, 0xff, 0xff, 0xff]);
    
	// Encode into 4 bytes.
	var size = 4;
	var result = new Uint8Array(size);
	var i = 0;
	while (i < 4) {
		//console.log(value);
		++i;
		result[size - i] = value % 256;
		value = Math.floor(value / 256);
	}
	return result;
};

NameCryptoState.dump_short = function(value) {
	if (value < 0)
		return new Uint8Array([0xff, 0xff]);
    
	// Encode into 2 bytes.
	var size = 2;
	var result = new Uint8Array(size);
	var i = 0;
	while (i < 2) {
		//console.log(value);
		++i;
		result[size - i] = value % 256;
		value = Math.floor(value / 256);
	}
	return result;
};

NameCryptoState.dump_int_little_endian = function(value) {
	if (value < 0)
		return new Uint8Array([0xff, 0xff, 0xff, 0xff]);
    
	// Encode into 4 bytes.
	var size = 4;
	var result = new Uint8Array(size);
	var i = 0;
	while (i < 4) {
		//console.log(value);
		result[i] = value % 256;
		value = Math.floor(value / 256);
		i++;
	}
	return result;
};

NameCryptoState.dump_short_little_endian = function(value) {
	if (value < 0)
		return new Uint8Array([0xff, 0xff]);
    
	// Encode into 2 bytes.
	var size = 2;
	var result = new Uint8Array(size);
	var i = 0;
	while (i < 2) {
		//console.log(value);
		result[i] = value % 256;
		value = Math.floor(value / 256);
		i++;
	}
	return result;
};

NameCryptoState.prototype.to_array = function() {
	var n = new Uint8Array(NameCryptoState.size);
	var n1 = NameCryptoState.dump_int(this.tv_sec);
	var n2 = NameCryptoState.dump_int(this.tv_usec);
	var n3 = NameCryptoState.dump_int(this.seq);
	var n4 = NameCryptoState.dump_int(this.rsvd);
	
	n.set(n1, 0);
	n.set(n2, 4);
	n.set(n3, 8);
	n.set(n4, 12);
	
	return n;
};

NameCryptoState.get_int = function(buffer) {
	var n = (buffer[0] << 24) + (buffer[1] << 16) + (buffer[2] << 8) + buffer[3];
	return n;
};

NameCryptoState.get_int_little_endian = function(buffer) {
	var n = (buffer[3] << 24) + (buffer[2] << 16) + (buffer[1] << 8) + buffer[0];
	return n;
};

NameCryptoState.prototype.from_array = function(buffer) {
	var n1 = buffer.subarray(0, 4);
	var n2 = buffer.subarray(4, 8);
	var n3 = buffer.subarray(8, 12);
	var n4 = buffer.subarray(12, 16);
	
	this.tv_sec = NameCryptoState.get_int(n1);
	this.tv_usec = NameCryptoState.get_int(n2);
	this.seq = NameCryptoState.get_int(n3);
	this.rsvd = NameCryptoState.get_int(n4);
};

NameCryptoState.prototype.valid = function(timeout) {
	var now = new Date();
	var d = this.tv_sec * 1000 + this.tv_usec / 1000;
	
	if (timeout < 0)
		return true;
	
	if (d > now)
		return false;
	
	if ((d + timeout) > now)
		return true;
	else
		return false;
}

var NameCrypto = function NameCrypto() {
	
};

NameCrypto.PK_AUTH_MAGIC = new Uint8Array([0x21, 0x44, 0x07, 0x65]);
NameCrypto.SK_AUTH_MAGIC = new Uint8Array([0x40, 0x96, 0x1c, 0x51]);
NameCrypto.AUTH_MAGIC_LEN = 4;

NameCrypto.authenticate_name_asymm = function(state, name, app_code, rsa) {
	state.update();
	
	var enc = new BinaryXMLEncoder();
	name.to_ccnb(enc);
	var n_name = enc.getReducedOstream();
	var n_app = DataUtils.toNumbersFromString(app_code);
	var n_state = state.to_array();
	
	var n_applen = NameCryptoState.dump_short(n_app.length);
	
	var n = new Uint8Array(n_name.length + n_app.length + n_state.length);
	n.set(n_name, 0);
	n.set(n_app, n_name.length);
	n.set(n_state, n_name.length + n_app.length);
	
	var hSig = rsa.signByteArrayWithSHA256(n).trim();
	var sig = DataUtils.toNumbers(hSig);
	
	var auth = new Uint8Array(sig.length + n_app.length + n_state.length + NameCrypto.AUTH_MAGIC_LEN + 2);
	
	auth.set(NameCrypto.PK_AUTH_MAGIC, 0);
	auth.set(n_applen, NameCrypto.AUTH_MAGIC_LEN);
	auth.set(n_app, NameCrypto.AUTH_MAGIC_LEN + 2);
	auth.set(n_state, NameCrypto.AUTH_MAGIC_LEN + 2 + n_app.length);
	auth.set(sig, NameCrypto.AUTH_MAGIC_LEN + 2 + n_app.length + n_state.length);
	
	return name.add(auth);
};

NameCrypto.verify_name_asymm = function(name) {
	var n_kl = name.components[name.components.length - 2];
	var n_crypto = name.components[name.components.length - 1];
	
	var dec = new BinaryXMLDecoder(n_kl);
	var kl = new KeyLocator();
	kl.from_ccnb(dec);
	
	var rsa = decodeSubjectPublicKeyInfo(kl.publicKey);
	
	var app_len = (n_crypto[NameCrypto.AUTH_MAGIC_LEN] << 8) + n_crypto[NameCrypto.AUTH_MAGIC_LEN + 1];
	var n_sig = n_crypto.subarray(NameCrypto.AUTH_MAGIC_LEN + 2 + app_len + NameCryptoState.size);
	var h_sig = DataUtils.toHex(n_sig);
	
	var name1 = (new Name(name.components.slice(0, name.components.length - 1)));
	var enc = new BinaryXMLEncoder();
	name1.to_ccnb(enc);
	var n_name1 = enc.getReducedOstream();
	
	var n_rest = n_crypto.subarray(NameCrypto.AUTH_MAGIC_LEN + 2, NameCrypto.AUTH_MAGIC_LEN + 2 + app_len + NameCryptoState.size);
	
	var n = new Uint8Array(n_name1.length + n_rest.length);
	n.set(n_name1, 0);
	n.set(n_rest, n_name1.length);
	
	return rsa.verifyByteArray(n, null, h_sig);
};

//NameCrypto.APP_ID_LEN = 16;
//NameCrypto.APP_KEY_LEN = 16;

NameCrypto.generate_symmetric_key = function(secret, app_code) {
	var h_app = DataUtils.stringToHex(app_code);
	var n_app = DataUtils.toNumbersFromString(app_code);
	var app_id = hex_sha256_from_bytes(n_app);
	
	var h_key = rstr2hex(
		rstr_hmac_sha256(
			str2rstr_utf8(secret), 
			DataUtils.hexToRawString(app_id)
		)
	);
	
	//return h_key.substring(0, NameCrypto.APP_KEY_LEN * 2);  // return a hex string
	return h_key;
};

NameCrypto.authenticate_name_symm = function(state, name, app_code, share_key) {
	state.update();
	
	var n_app = DataUtils.toNumbersFromString(app_code);
	var n_state = state.to_array();
	
	var enc = new BinaryXMLEncoder();
	name.to_ccnb(enc);
	var n_name = enc.getReducedOstream();
	
	var m = new Uint8Array(n_name.length + n_app.length + n_state.length);
	m.set(n_name, 0);
	m.set(n_app, n_name.length);
	m.set(n_state, n_name.length + n_app.length);
	
	var n_mac = DataUtils.toNumbers(
		rstr2hex(
			rstr_hmac_sha256(
				DataUtils.hexToRawString(share_key), // shared key is hex string
				DataUtils.hexToRawString(DataUtils.toHex(m))
			)
		)
	);
	//console.log(DataUtils.toHex(n_mac));
	
	var n_applen = NameCryptoState.dump_short(n_app.length);
	
	var n = new Uint8Array(NameCrypto.AUTH_MAGIC_LEN + 2 + n_app.length + n_state.length + n_mac.length);
	n.set(NameCrypto.SK_AUTH_MAGIC, 0);
	n.set(n_applen, NameCrypto.AUTH_MAGIC_LEN);
	n.set(n_app, NameCrypto.AUTH_MAGIC_LEN + 2);
	n.set(n_state, NameCrypto.AUTH_MAGIC_LEN + 2 + n_app.length);
	n.set(n_mac, NameCrypto.AUTH_MAGIC_LEN + 2 + n_app.length + n_state.length);
	
	return name.add(n);
};

NameCrypto.verify_name_symm = function(name, secret, timeout, app_code) {
	var n_crypto = name.components[name.components.length - 1];
	
	var app_len = (n_crypto[NameCrypto.AUTH_MAGIC_LEN] << 8) + n_crypto[NameCrypto.AUTH_MAGIC_LEN + 1];
	var n_app = n_crypto.subarray(NameCrypto.AUTH_MAGIC_LEN + 2, NameCrypto.AUTH_MAGIC_LEN + 2 + app_len);
	var share_key = NameCrypto.generate_symmetric_key(secret, app_code);
	
	var n_state = n_crypto.subarray(NameCrypto.AUTH_MAGIC_LEN + 2 + app_len, 
		NameCrypto.AUTH_MAGIC_LEN + 2 + app_len + NameCryptoState.size);
	var state = new NameCryptoState();
	state.from_array(n_state);
	
	var ret = state.valid(timeout);
	if (!ret)
		return false;
	
	var n_mac = n_crypto.subarray(NameCrypto.AUTH_MAGIC_LEN + 2 + app_len + NameCryptoState.size);
	
	var name1 = (new Name(name.components.slice(0, name.components.length - 1)));
	var enc = new BinaryXMLEncoder();
	name1.to_ccnb(enc);
	var n_name1 = enc.getReducedOstream();
	
	var m = new Uint8Array(n_name1.length + n_app.length + n_state.length);
	m.set(n_name1, 0);
	m.set(n_app, n_name1.length);
	m.set(n_state, n_name1.length + n_app.length);
	
	var mac = DataUtils.toNumbers(
		rstr2hex(
			rstr_hmac_sha256(
				DataUtils.hexToRawString(share_key), // shared key is hex string
				DataUtils.hexToRawString(DataUtils.toHex(m))
			)
		)
	);
	//console.log(DataUtils.toHex(mac));
	
	if (mac.length != n_mac.length)
		return false;
	
	ret = true;
	for (var i = 0; i < mac.length; i++) {
		if (mac[i] != n_mac[i]) {
			ret = false;
			break;
		}
	}
	
	return ret;
};
