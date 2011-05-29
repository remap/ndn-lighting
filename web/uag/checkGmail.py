import email, getpass, imaplib, os
import database as db
import config as cfg
import image as img
import analyze

def getEmail():
	detach_dir = cfg.emailImagePath # directory where to save attachments
	user = cfg.emailUser 
	pwd = cfg.emailPass 

	# connecting to the gmail imap server
	m = imaplib.IMAP4_SSL("imap.gmail.com")
	m.login(user,pwd)
	m.select(cfg.emailLabel) # here you a can choose a mail box like INBOX instead
	# use m.list() to get all the mailboxes
	#print m.list()
	#resp, items = m.search(None, "ALL") 
	# you could filter using the IMAP rules here (check http://www.example-code.com/csharp/imap-search-critera.asp)
	resp, items = m.search(None, '(FROM "ucla.edu")')
	
	authorName = ""
	authorEmail = ""
	authorTitle = ""
	authorDescription = ""
	
	items = items[0].split()
	
	if(items):
		for emailid in items:
		    resp, data = m.fetch(emailid, "(RFC822)")
		    #"`(RFC822)`" means "get the whole stuff", but you can ask for headers only, etc
		    email_body = data[0][1] # getting the mail content
		    mail = email.message_from_string(email_body) # parsing the mail content to get a mail object

		    #Check if any attachments at all
		    if mail.get_content_maintype() != 'multipart':
		        continue
			
		   #print mail["From"] +":" + mail["Subject"]
		    
		    authorTitle = mail["Subject"]
		    authorName = mail["From"].split('<',2)[0]
		    #print authorName
		    authorEmail = mail["From"].split('<',2)[1].rstrip(">")
		   	#print authorEmail
		   	
		    for part in mail.walk():
		        # multipart are just containers, so we skip them
		        if part.get_content_maintype() == 'multipart':
		            continue
		        
		        if part.get_content_maintype() == 'text':
		        	message = part.get_payload()
		        	#print "body :"+ message
		        	authorDescription = message
		        	#print "Name :"+ authorName
		        	#print "eMail:"+ authorEmail
		        	#print "title:"+authorTitle
		        	#print "desc :"+ authorDescription
		        	continue
	
		        # is this part an attachment ?
		        if part.get('Content-Disposition') is None:
		            continue
	
		        filename = part.get_filename()
		        counter = 1
	
		        # if there is no filename, we create one with a counter to avoid duplicates
		        if not filename:
		            filename = 'part-%03d%s' % (counter, 'bin')
		            counter += 1
				
		        att_path = os.path.join(detach_dir, authorEmail+"_"+filename)
	
		        #Check if its already in the email directory
		        if not os.path.isfile(att_path) :
		            # new data, save info to database
		            lastID = db.writeDatabase(authorName, authorEmail,authorTitle, authorDescription)
		            last = db.getEntryFromID(lastID)
		        	
		            #write to normal image directory w/ UID prefix
		            imgPath = os.path.join(cfg.imageFilePath, str(last['_id'])+"_"+filename)
		            obj = db.updateFilenameFromID(last['_id'],str(last['_id'])+"_"+filename)
		            
		            eImgPath = os.path.join(cfg.emailImagePath, authorEmail+"_"+filename)
		            
		            # write to email image directory, so we don't re-insert same image later
		            fp = open(eImgPath, 'wb')
		            fp.write(part.get_payload(decode=True))
		            fp.close()
		            
		            fp = open(imgPath, 'wb')
		            fp.write(part.get_payload(decode=True))
		            fp.close()
		            
		            obj = img.resize(last['_id'])
		            obj = analyze.histogram(last['_id'], cfg.analysisMode)
		           

if __name__ == "checkGmail":
    print("checkGMail instantiating...")
    #model()
    
if __name__ == "__main__":
    print("checkGMail running")
    getEmail()