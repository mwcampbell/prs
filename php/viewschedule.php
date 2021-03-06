<?
require_once ("common.php");
check_user ();
db_connect ();
$date_template = "%a %d-%b-%y %T";

// This script is called via get from updatetimeslot.php
// and via post from itself.

if ($_SERVER["REQUEST_METHOD"] == "GET") {
	if ($_GET["start_date"])
		$start_date = $_GET["start_date"];
	if ($_GET["end_date"])
		$end_date = $_GET["end_date"];
} // end if "get" request
else {
	$start_date = $_POST["start_date"];
	$end_date = $_POST["end_date"];
} // end if "POst" request.


if (!$start_date || !$end_date) {
	html_start ("View Schedule");
?>
<form name = "view_schedule" action="viewschedule.php" method="post">
<div>
<label for="start_date">Enter start date :</label>
<input type = text name="start_date" id="start_date">
</div>
<div>
<label for="end_date">Enter end date:</label>
<input type = text name="end_date" id="end_date">
</div>
<input type="submit" value="View Schedule">
</form>
<?
	html_end ();
	exit ();
} // end if no dates specified

$start_date = strtotime ($start_date);
$end_date = strtotime ($end_date);
if ($start_date == -1 || $end_date == -1) {
	html_error ("Start or end date out of range.");
	exit ();
}
$start_time = strftime ($date_template, $start_date);
$end_time = strftime ($date_template, $end_date);
$title = "Schedule - $start_time - $end_time";
html_start ($title);
?>
<table>
<tr>
<th>Start Date</th>
<th>End Date</th>
<th>Repetition</th>
<th>Playlist Template</th>
<th>Fallback Template</th>
<th>End Prefade</th>
<th>Actions</th>
</tr>
<?
	$cur_time = $start_date;
	while ($cur_time < $end_date) {
		$daylight = date ("I", $cur_time);
		$query = "select time_slot_id, schedule.template_id, template_name, repeat_events,
          handle_overlap, artist_exclude, recording_exclude, start_time,
          length, repetition, fallback_id,
          end_prefade from playlist_template, schedule
          where playlist_template.template_id = schedule.template_id and
          ((repetition = 0 and start_time <= $cur_time and start_time+length > $cur_time) or
          (repetition != 0 and start_time <= $cur_time and mod($cur_time-start_time-(daylight*3600)+$daylight, repetition) < length))
          order by time_slot_id desc";
		$res = db_query ($query);
		$row = mysql_fetch_assoc ($res);
		if (!$row) {
			$cur_time += 300;
			continue;
		}
		$time_slot_id = $row["time_slot_id"];
		$start_time = $row["start_time"];
		$length = $row["length"];
		$repetition = $row["repetition"];
		$template_id = $row["template_id"];
		$fallback_id = $row["fallback_id"];
		$end_prefade = $row["end_prefade"];
		$end_time = $start_time+$length;
		$start_time = strftime ($date_template, $start_time);
		$end_time = strftime ($date_template, $end_time);
?>
<tr>
<form name="modify_timeslot" action="updatetimeslot.php" method="post">
<input type = "hidden" name = "time_slot_id" id="time_slot_id" value="<? echo $time_slot_id; ?>">
<td>
<input type="text" name="start_time" id="start_time" length="30" value="<? echo $start_time; ?>">
</td>
<td>
<input type="text" name="end_time" id="end_time" length="30" value="<? echo $end_time; ?>">
</td> 
<td>
<?
display_repetition_list ($repetition);
?>
</td>
<td>
<select name="template_id" id="template_id">
<? display_template_list ($template_id); ?>
</select>
</td>
<td>
<select name="fallback_id" id="fallback_id">
<? display_template_list ($fallback_id); ?>
</select>
</td>
<td>
<input type = "text" name="end_prefade" id="end_prefade" value="<? echo $end_prefade; ?>">
</td>
<td>
<input type="hidden" name="action" id="action" value="2">
<input type="submit" value="Update Time Slot">
</form>
<form name = "delete_time_slot" action="updatetimeslot.php" method="post">
<input type="hidden" name="time_slot_id" value="<? echo $time_slot_id ?>">
<input type="hidden" name="action" id="action" value="1">
<input type="submit" value="Delete Time Slot">
</form>
</td>
<?
	  mysql_free_result ($res);
	  $cur_time += $length;
}
?>
</table>
<a href = "main.php">Back to Main Menu</a>
<?
html_end ();
exit ();
