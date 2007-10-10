<?
require_once ("common.php");
check_user ();
db_connect ();

/* Ensure that event_anchor_position has something in it */

if ($_POST["event_anchor_position"])
	$event_anchor_position = $_POST["event_anchor_position"];
else
	$event_anchor_position = "0";

if ($_POST["event_channel_name"])
	$event_channel_name = $_POST["event_channel_name"];
else
	$event_channel_name = $_POST["event_name"];

if ($_POST["insert_event"]) {
	/* Move existing events to make room */

	$query = "update playlist_event set event_number = event_number+1 where " .
	"event_number >= " . $_POST["event_number"] . " and template_id = " . $_POST["template_id"];
	db_query ($query);
}

/* See if event exists */

$query = "select * from playlist_event where event_number = " . $_POST["event_number"] .
	" and template_id = " . $_POST["template_id"];
$res = db_query ($query);
if (mysql_numrows ($res) > 0) {
	$query = 'update playlist_event ' .
        'set template_id = ' . $_POST["template_id"] . ', ' .
	'event_number = ' . $_POST["event_number"] . ', ' .
        'event_name = "' . $_POST["event_name"] . '", ' .
	'event_type = "' . $_POST["event_type"] . '", ' .
        'event_channel_name = "' . $event_channel_name . '", ' .
	'event_level = "' . $_POST["event_level"] . '", ' .
        'event_anchor_event_number = ' . $_POST["event_anchor_event_number"] . ', ' .
        'event_anchor_position = ' . $event_anchor_position . ', ' .
        'event_offset = ' . $_POST["event_offset"] . ', ' .
        'detail1 = "' . $_POST["detail1"] . '", ' .
	'detail2 = "' . $_POST["detail2"] . '", ' .
        'detail3 = "' . $_POST["detail3"] . '", ' .
	'detail4 = "' . $_POST["detail4"] . '", ' .
        'detail5 = "' . $_POST["detail5"] . '" ' .
        'where template_id = ' . $_POST["template_id"] . ' ' .
	'and event_number = ' . $_POST["event_number"];
}
else {
	$query = "insert into playlist_event
	(template_id, event_number,
	event_name, event_type,
	event_channel_name, event_level,
	event_anchor_event_number,
	event_anchor_position,
	event_offset,
	detail1, detail2,
	detail3, detail4,
	detail5) values (" . 
	$_POST["template_id"] . ', ' . $_POST["event_number"] . ', ' .
	'"' . $_POST["event_name"] . '", "' . $_POST["event_type"] . '", ' .
	'"' . $event_channel_name . '", ' . $_POST["event_level"] . ', ' .
	$_POST["event_anchor_event_number"] . ', ' .
	$event_anchor_position . ', ' . $_POST["event_offset"] . ', ' .
	'"' . $_POST["detail1"] . '", "' . $_POST["detail2"] . '", ' .
	'"' . $_POST["detail3"] . '", "' . $_POST["detail4"] . '", ' .
	'"' . $_POST["detail5"] . '")';
}
db_query ($query);
if ($_POST["update_event"] || $_POST["insert_event"]) {
	redirect ("edittemplate.php?template_id=" . $_POST["template_id"]);
}
else {
	redirect ("addevents.php?template_id=" . $_POST["template_id"]);
}
?>
