CREATE TABLE levels (
	channel			INT				NOT NULL,
	autoop			INT	( 3 )		NOT NULL DEFAULT '300',
	autohalfop		INT ( 3 )		NOT NULL DEFAULT '200',
	autovoice		INT ( 3 )		NOT NULL DEFAULT '100',
	autodeop		INT ( 3 )		NOT NULL DEFAULT '-1',
	autodehalfop	INT ( 3 )		NOT NULL DEFAULT '-1',
	autodevoice		INT ( 3 )		NOT NULL DEFAULT '-1',
	nojoin			INT ( 3 )		NOT NULL DEFAULT '-1',
	invite			INT ( 3 )		NOT NULL DEFAULT '300',
	akick			INT ( 3 )		NOT NULL DEFAULT '300',
	`set`			INT ( 3 )		NOT NULL DEFAULT '500',
	clear			INT	( 3 )		NOT NULL DEFAULT '500',
	unban			INT ( 3 )		NOT NULL DEFAULT '300',
	opdeop			INT ( 3 )		NOT NULL DEFAULT '300',
	halfopdehalfop	INT ( 3 )		NOT NULL DEFAULT '200',
	voicedevoce		INT ( 3 ) 		NOT NULL DEFAULT '300',
	`acc-list`		INT ( 3 )		NOT NULL DEFAULT '100',
	`acc-change`	INT ( 3 )		NOT NULL DEFAULT '500',
	
	PRIMARY KEY ( channel ),
	FOREIGN KEY ( channel ) REFERENCES channel ( id )
		ON DELETE CASCADE ON UPDATE CASCADE
) engine=InnoDB;