<?
session_start ();
$html_started = 0;
$html_title_prefix = "PRS Manager: ";
if ($_SESSION["station_name"])
  $html_title_prefix .= $_SESSION["station_name"] . ": ";

function
html_start ($title)
{
	global $html_started, $html_title_prefix;

	if ($html_started)
		return;

	$html_started = 1;
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01//EN">
<html lang="en">
<head>
<title><? echo $html_title_prefix . $title ?></title>
</head>
<body>
<h1><? echo $title ?></h1>
<?
}

function
html_end ()
{
?>
</body>
</html>
<?
}

function
html_error ($message)
{
	html_start ("Error");
	echo ("<p>$message</p>\n");
	html_end ();
	exit ();
}

function
db_connect ()
{
	mysql_connect ($_SESSION["DB_HOST"], $_SESSION["DB_USER"], $_SESSION["DB_PASSWORD"])
		or html_error ("Could not open database connection to " . $_SESSION["DB_HOST"] . ".");
	mysql_select_db ($_SESSION["DB_NAME"]) or html_error ("Could not select database " . $_SESSION["DB_NAME"] . ".");
}

function
db_query ($query)
{
	$result = mysql_query ($query);

	if (!$result) {
		$error_string =  "Database query failed: " . mysql_error() . "</P>\n\n" .
			"<P>The query which failed was:</P>\n\n" .
			"<P><PRE>" . htmlspecialchars($query, ENT_QUOTES) . "</PRE>";
		html_error ($error_string);
	} // end if query failed

	return $result;
}

function
check_user ()
{
	if (!$_SESSION["user_username"] || $_SESSION["user_type"] != "admin")
	{
		html_error ("Access denied.");
	}
}

function
redirect ($relative_url)
{
	header ("HTTP/1.1 303 See Other");
	header ("Location: http://" . $_SERVER["SERVER_NAME"] .
		dirname ($_SERVER["PHP_SELF"]) . "/$relative_url");
	exit ();
}


function
display_template_list ($selected_template, $exclude_selected=false)
{
        $res = db_query ("select template_id, template_name from playlist_template order by template_name");
        echo "<option value=\"-1\">None Selected</option>\n";
	while ($row = mysql_fetch_assoc ($res)) {
        	$id = $row["template_id"];
        	$name = $row["template_name"];
		if (!$exclude_selected or ($id != $selected_template)) {
	        	echo "<option value=\"$id\"";
			if ($id == $selected_template)
				echo " selected";
			echo ">$name</option>\n";
		}
        }
        mysql_free_result ($res);
}

function
display_repetition_list ($selected_repetition)
{
	$repetition_table = array (
		0 => "One Time Only",
		3600 => "Hourly",
		86400 => "Daily",
		604800 => "Weekly",
		1209600 => "2 Weeks",
		1814400 => "3 Weeks",
		2419200 => "4 Weeks"
	);

	echo "<select name=\"repetition\" id=\"repetition\">\n";
	reset ($repetition_table);
	while (list ($key, $value) = each ($repetition_table)) {
		echo "<option value=\"$key\"";
		if ($selected_repetition == $key)
			echo " selected";
		echo ">$value</option>\n";
	} // end while list of repetition types
	echo "</select>\n";
}
?>
