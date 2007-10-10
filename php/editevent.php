<?
require_once ("common.php");
check_user ();
db_connect ();

// This script is called via Get from addevents.php and from a form via POst
// from edittemplate.php and viewtemplate.php.  So we need to check for
// the correct version of each variable and assign it to a copy in this script.
// NOte that we only check for variables passed using both methods.

if ($_SERVER["REQUEST_METHOD"] == "GET")
{
// Template ID is always set from addevents.php
	$template_id = $_GET["template_id"];
	if ($_GET["event_number"])
		$event_number = $_GET["event_number"];
	if ($_GET["insert_event"])
		$insert_event = $_GET["insert_event"];
// We know that event_type will be set via get from addevents.php
	$event_type = $_GET["event_type"];
} else if ($_SERVER["REQUEST_METHOD"] == "POST")
{
	$template_id = $_POST["template_id"];
	$event_number = $_POST["event_number"];
	$event_name = $_POST["event_name"];
	$event_type = $_POST["event_type"];

// NOte that the following post variables are sent but not used
// (maybe they should be?):

// launched_from
// event_channel_name
// event_level
// event_anchor_event_number
// event_anchor_position
// event_offset

} // end if post request

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

if (($event_type == "random") or ($event-type == "simple_random"))
{
	$categories = array ();
	$res = db_query ("select category_name from category");
	while ($row = mysql_fetch_assoc ($res))
		$categories[] = $row["category_name"];
	mysql_free_result ($res);
} // end if random or simple random event

// Set page title
if ($update_event)
	$page_title = "Edit ";
else
	$page_title = "Add ";

switch ($event_type)
{
	case "simple_random":
		$page_title .= "Simple ";
	case "random":
		$page_title .= "Random Event";
		break;
	case "url":
		$page_title .= "URL Event";
		break;
	case "path":
		$page_title .= "Path Event";
} // end switch

html_start ($page_title);
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
// Get correct user input based on event_type
switch ($event_type)
{
	case "random":
	case "simple_random":
		for ($i = 1; $i <= min (count ($categories), 5); $i++)
		{
			echo ("<label for=\"detail$i\">Category $i:</label>\n");
			echo ("<select name=\"detail$i\" id=\"detail$i\">\n");
			echo ("<option value=\"\">None Selected</option>\n");
			foreach ($categories as $category)
			{
				print ("<option");
				if (($i == 1 && $category == $_POST["detail1"]) ||
				    ($i == 2 && $category == $_POST["detail2"]) ||
				    ($i == 3 && $category == $_POST["detail3"]) ||
				    ($i == 4 && $category == $_POST["detail4"]) ||
				    ($i == 5 && $category == $_POST["detail5"]))
					print (" selected=on");
				print(">$category</option>\n");
			} // end foreach
			print ("</select><br>\n");
		} // end for
		break; // end if random or simple_random event

	case "url":
		echo '<label for="detail1">Enter URL:</label>' . "\n";
		echo '<input type = "text" name = "detail1" id="detail1" ';
		echo 'value = "' . htmlspecialchars ($_POST["detail1"]) . '"><BR>' . "\n";
		echo '<label for="detail2">Enter length in seconds:</label>' . "\n";
		echo '<input type = "text" name="detail2" id="detail2" ';
		echo 'value = "' . $_POST["detail2"] . '"><BR>' . "\n";
		echo '<label for="detail3">Enter archive File Name:</label>' . "\n";
		echo '<input type = "text" name="detail3" id="detail3" ';
		echo 'value = "' . htmlspecialchars($_POST["detail3"]) . '"><BR>' . "\n";
		break; // end if URL event.

	case "path":
		echo '<label for="detail1">Enter File Name:</label>' . "\n";
		echo '<input type = "text" name = "detail1" id="detail1" ';
		echo 'value = "' . htmlspecialchars($_POST["detail1"]) . '">' . "\n";
} // end switch event_type

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
