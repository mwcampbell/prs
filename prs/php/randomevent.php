<?
require_once ("common.php");
check_user ();
db_connect ();

/* Get the next available event number for this template */

if (!$event_number) {
	$query = "select max(event_number) max from playlist_event where template_id = $template_id";
	$res = db_query ($query);
	$row = mysql_fetch_assoc ($res);
	$event_number = $row["max"] + 1;
	mysql_free_result ($res);
}
else {
	$update_event = "yes";
}
if (!$event_name)
        $event_name = "event $event_number";
$categories = array ();
$res = db_query ("select category_name from category");
while ($row = mysql_fetch_assoc ($res))
	$categories[] = $row["category_name"];
mysql_free_result ($res);
html_start ("Add " . (($event_type == "simple_random") ? "Simple Random" : "Random") . " Event");
?>
<form name="event" action="createevent.php" method="post">
<input type = hidden name="insert_event" id="insert_event" value="<? echo $insert_event ?>">
<input type = hidden name="update_event" id="insert_event" value="<? echo $update_event ?>">
<div>
<label for="event_name">Name:</label>
<input type="text" name="event_name" id="event_name" value="<? echo $event_name ?>"><br>
<input type="hidden" name="event_anchor_event_number" value="-1">
<input type="hidden" name="event_anchor_position" value="1">
<input type="hidden" name="event_offset" value="0.0">
<input type="hidden" name="event_level" value="1.0">
<?

for ($i = 1; $i <= min (count ($categories), 5); $i++)
{
	echo ("<label for=\"detail$i\">Category $i:</label>\n");
	echo ("<select name=\"detail$i\" id=\"detail$i\">\n");
	echo ("<option value=\"\">None Selected</option>\n");
	foreach ($categories as $category)
	{
		print ("<option");
		if (($i == 1 && $category == $detail1) ||
		    ($i == 2 && $category == $detail2) ||
		    ($i == 3 && $category == $detail3) ||
		    ($i == 4 && $category == $detail4) ||
		    ($i == 5 && $category == $detail5))
			print (" selected=on");
		print(">$category</option>\n");
	}
	print ("</select><br>\n");
}
foreach (array ("template_id", "event_number", "event_type") as $field)
{
	echo ("<input type=\"hidden\" name=\"$field\" value=\"" . $$field . "\">\n");
}
?>
<input type="submit" value="Add Event">
</div>
</form>
<?
html_end ();
?>
