BEGIN TRANSACTION;
CREATE TABLE DSYNC_ZONE 
(
	ZoneID Integer PRIMARY KEY NOT NULL,
	ZoneName Text NOT NULL,
	ZoneAlarm VarChar NULL,
  State int, ParentName varchar);
CREATE TABLE DSYNC_NAMESERVER
(
	NameServerID Integer PRIMARY KEY NOT NULL,
	NameServerName Text NULL,
	NameServerIP Varchar NULL,
	ZoneFK Integer NOT NULL,
  Verified INTEGER);
CREATE TABLE DSYNC_DNSKEY
(
	DnskeyID Integer PRIMARY KEY NOT NULL,
	RRSetFK Integer NOT NULL,
	Protocol SmallInt NOT NULL,
	Algorithm SmallInt NOT NULL,
	TTL Integer NULL,
	B64Key Text NULL,
	RData BLOB NOT NULL
);
CREATE TABLE DSYNC_DS
(
	DsID Integer PRIMARY KEY NOT NULL,
	Protocol SmallInt NOT NULL,
	Algorithm SmallInt NOT NULL,
	TTL Integer NULL,
	B16Digest Text NULL,
	RData BLOB NOT NULL,
	RRSetFK Integer NOT NULL
);
CREATE TABLE DSYNC_RRSET
(
	RRSetID Integer PRIMARY KEY NOT NULL,
	ZoneFK Integer NOT NULL,
	LastSeen Integer NOT NULL,
  NameServerFK integer);
CREATE TABLE DSYNC_RRSIG
(
	RRSigID Integer PRIMARY KEY NOT NULL,
	RRSetFK Integer NOT NULL,
	LastSeen Integer NOT NULL,
	RData BLOB NULL
);
CREATE TABLE DSYNC_HISTORY
(
	HistoryID Integer PRIMARY KEY NOT NULL,
	Timestamp Integer NOT NULL,
	RRSigFK Integer NOT NULL,
	RRSetFK Integer NOT NULL,
  NameServerIP Varchar);
COMMIT;
