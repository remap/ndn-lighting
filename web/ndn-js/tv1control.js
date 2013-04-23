
    var ndn = new NDN({port:9696,host:"ndnucla-staging.dyndns.org"});
    
    ndn.onopen = function() {
    	//document.getElementById("testBtn").disabled = false;
    	//document.getElementById("name").innerHTML = b64tohex(globalKeyManager.certificate);
    };
    
	ndn.transport.connectWebSocket(ndn);
	
	function sendCommand(name) {
		var template = new Interest();
		template.interestLifetime = 100;
		
		//var rgb_val = document.getElementById('rgb').value;
		
		var publicKeyBytes = DataUtils.toNumbers(globalKeyManager.publicKey);
		var locator = new KeyLocator(publicKeyBytes, KeyLocatorType.KEY);
		var enc = new BinaryXMLEncoder();
		locator.to_ccnb(enc);
		var n_loc = enc.getReducedOstream();
		name.add(n_loc);
		
		var app_code = "TV1 Sequencer";
		
		var rsa = new RSAKey();
		rsa.readPrivateKeyFromPEMString(globalKeyManager.privateKey);
		
		var state = new NameCryptoState();
		
		NameCrypto.authenticate_name_asymm(state, name, app_code, rsa);
		
		ndn.expressInterest(name, null, template);
		
		//document.getElementById("name").innerHTML = name.getName();
		
		//var ret1 = NameCrypto.verify_name_asymm(name);
		
		//document.getElementById("name").innerHTML += '<p>' + ret1 + '</p>';
	}