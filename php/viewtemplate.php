<?
require_once ("common.php");
check_user ();
db_connect ();

/* If we don't have a template id, let user pick a template */

if (!$template_id) {
	html_start ("Select Playlist Template");
?>
<form name = "select_template" action="edittemplate.php" method="post">
<select name="template_id">
<? display_template_list (-1); ?>
</select>
<input type = "submit" value="Edit Template">
</form>
?>
	html_end ();
	exit ();
}

/* Get list of events in specified template */

$query = "select * from playlist_event where template_id = $template_id";
$res = db_query ($query);
?>
<table>
<tr>
<th>Event Name</th>
<th>Event Type</th>
<th>Details</th>
</tr>
while ($row = mysql_get_assoc ($res)) {
	$template_id = $row["template_id"];
	$event_number = $row["event_number"];
	$event_name = $row["event_name"];
	$event_channel_name = $row["event_channel_name"];
	$event_level = $row["event_level"];
	$event_offset = $row["event_offset"];
	$event_anchor_event_number = $row["event_anchor_event_number"];
	$event_anchor_position = $row["event_anchor_position"];
	$detail1 = $row["detail1"];
	$detail2 = $row["detail2"];
	$detail3 = $row["detail3"];
	$detail4 = $row["detail4"];
	$detail5 = $row["detail5"];
	echo "<td>$event_name</td>\n";
	echo "<td>$event_type</td>\n";
	echo "<td>$detail1";
	if ($detail2)
		echo ", $detail2";
	if ($detail3)
		echo ", $detail3";
	if ($detail4)
		echo ", $detail4";
	if ($detail5)
		echo ", $detail5";
	echo "</td>";
?>
<form name="edit_event" method="post"
<?
	 if ($event_type == "random" || $event_type == "simple_random")
		 echo " action=\"randomevent.php\">";
	if ($event_type == "url")
		 echo " action=\"urlevent.php\">";
	if ($event_type == "path")
		 echo " action=\"pathevent.php\">";
?>
<input type = hidden name="template_id" id="template_id" value="<? echo $template_id ?>">
<input type = hidden name="event_number" id="event_number" value="<? echo $event_number ?>">
<input type = hidden name="event_name" id="event_name" value="<? echo $event_name ?>">
<input type = hidden name="event_channel_name" id="event_channel_name" value="<? echo $event_channel_name ?>">
<input type = hidden name="event_level" id="event_level" value="<? echo $event_level ?>">


		 
	}
