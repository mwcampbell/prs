<?
require_once ("common.php");
check_user ();
db_connect ();
$date_template = "%a %d-%b-%y %T";

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
}
$start_date = strtotime ($start_date);
$end_date = strtotime ($end_date);
if ($start_date == -1 || $end_date == -1) {
	html_error ("Start or end date out of range.");
	exit ();
}
$query = "select * from schedule where start_time >= $start_date and start_time <= $end_date";
$start_date = strftime ($date_template, $start_date);
$end_date = strftime ($date_template, $end_date);
$title = "Schedule - $start_date - $end_date";
html_start ($title);
$res = db_query ($query);
html_start ("Edit Schedule");
?>
<table>
<tr>
<th>Start Time</th>
<th>End Date</th>
<th>Playlist Template</th>
<th>Actions</th>
</tr>
<?
while ($row = mysql_fetch_assoc ($res)) {
        $time_slot_id = $row["time_slot_id"];
	$start_time = $row["start_time"];
        $end_time = $row["end_time"];
        $template_id = $row["template_id"];
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
<select name="template_id" id="template_id">
<? display_template_list ($template_id); ?>
</select>
</td>
<td>
<input type="hidden" name="action" id="action" value="2">
<input type="submit" value="Modify Time Slot">
</form>
<form name = "delete_time_slot" action="updatetimeslot.php">
<input type="hidden" name="time_slot_id" value="<? echo $time_slot_id ?>">
<input type="hidden" name="action" id="action" value="1">
<input type="submit" value="Delete Time Slot">
</form>
</td>
<?
}
mysql_free_result ($res);
?>
</table>
<a href = "schedule.php">Back to Schedule Administration page</a>
<?
html_end ();
exit ();
