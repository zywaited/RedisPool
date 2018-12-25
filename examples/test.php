<?php
$r = new Redis();
$r->connect('/tmp/socket/rs.sock');
//var_dump($r->auth('123456'));
var_dump($r->select(0));
var_dump($r->get('waited'));
var_dump($r->get('waited_test'));

/**
	bool(true)
	string(1) "1"
	bool(false)
*/