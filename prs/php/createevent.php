<?
require_once ("common.php");
check_user ();
db_connect ();

/* Ensure that event_anchor_position has something in it */

if (!$event_anchor_position)
	$event_anchor_position = "0";

if (!$event_channel_name)
	$event_channel_name = $event_name;

if ($insert_event) {
	/* Move existing events to make room */

	$query = "update playlist_event set event_number = event_number+1 where event_number >= $event_number and template_id = $template_id";
	db_query ($query);
}

/* See if event exists */

$query = "select * from playlist_event where event_number = $event_number and template_id = $template_id";
$res = db_query ($query);
if (mysql_numrows ($res) > 0) {
	$query = "update playlist_event
        set template_id = $template_id, event_number = $event_number,
        event_name = \"$event_name\", event_type = \"$event_type\",
        event_channel_name = \"$event_channel_name\", event_level = \"$event_level\",
        event_anchor_event_number = $event_anchor_event_number,
        event_anchor_position = $event_anchor_position,
        event_offset = $event_offset,
        detail1 = \"$detail1\", detail2 = \"$detail2\",
        detail3 = \"$detail3\", detail4 = \"$detail4\",
        detail5 = \"$detail5\"
        where template_id = $template_id and event_number = $event_number";
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
}
db_query ($query);
if ($update_event || $insert_event) {
	redirect ("edittemplate.php?template_id=$template_id");
}
else {
	redirect ("addevents.php?template_id=$template_id");
}
?>