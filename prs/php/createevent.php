<?
require_once ("common.php");
check_user ();
db_connect ();

/* Ensure that event_anchor_position has something in it */

if (!$event_anchor_position)
	$event_anchor_position = "0";

if (!$event_channel_name)
	$event_channel_name = $event_name;

$query = "insert into playlist_event
	(template_id, event_number,
	event_name, event_type,
	event_channel_name, event_level,
	event_anchor_event_number,
	event_anchor_position,
	event_offset,
	detail1, detail2,
	detail3, detail4,
	detail5) values
	($template_id, $event_number,
	'$event_name', '$event_type',
	'$event_channel_name', $event_level,
	$event_anchor_event_number,
	$event_anchor_position,
	$event_offset,
	'$detail1', '$detail2',
	'$detail3', '$detail4',
	'$detail5')";
db_query ($query);
redirect ("addevents.php?template_id=$template_id");
?>
