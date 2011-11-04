from pyccn import NameCrypto, CCN, Name

handle = CCN.CCN()
key = handle.getDefaultKey()
del handle

fixture_key ="1234"
app_name="lightingController"
name = Name.Name(["name"])
app_key = NameCrypto.generate_application_key(fixture_key, app_name)
sender_state = NameCrypto.new_state()
receiver_state = NameCrypto.new_state()

auth_name1 = NameCrypto.authenticate_command(sender_state, name, app_name, app_key)
print("ccnx:" + str(auth_name1))

auth_name2 = NameCrypto.authenticate_command_sig(sender_state, name, app_name, key)
print("ccnx:" + str(auth_name2))

result = NameCrypto.verify_command(receiver_state, auth_name1, 10000, fixture_key=fixture_key, pub_key=key)
print(result)

result = NameCrypto.verify_command(receiver_state, auth_name2, 10000, fixture_key=fixture_key, pub_key=key)
print(result)
