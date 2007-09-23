<?
require_once ("config.php");
session_start ();
$html_started = 0;
$html_title_prefix = "PRS Manager: ";

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
startElement ($parser, $name, $attrs) {
	global $DB_HOST, $DB_NAME, $DB_USER, $DB_PASSWORD;

	if ($name == "DB") {
		$DB_USER = $attrs["USER"];
		$DB_HOST = $attrs["HOST"];
		$DB_PASSWORD = $attrs["PASSWORD"];
		$DB_NAME = $attrs["NAME"];
	}
}

function
endElement ($parser, $name) {
}

function
parse_config_file ($filename) {
	$parser = xml_parser_create ();
	xml_set_element_handler ($parser, "startElement", "endElement");
	
	if (!$fp = fopen ($filename, "r")) {
		html_error ("Configuration file not found.");
		exit ();
	}
	while ($data = fread ($fp, 4096)) {
		xml_parse ($parser, $data, feof($fp));
	}
}

function
db_connect ()
{
	global $DB_HOST, $DB_NAME, $DB_USER, $DB_PASSWORD, $station;
	mysql_connect ($DB_HOST, $DB_USER, $DB_PASSWORD)
		or html_error ("Could not open database connection.");
	mysql_select_db ($DB_NAME) or html_error ("Could not select database.");
}

function
db_query ($query)
{
	$result = mysql_query ($query);

	if (!$result)
		html_error ("Database query failed: " . mysql_error ());

	return $result;
}

function
check_user ()
{
	global $user_username, $user_type;

	if (!$user_username || $user_type != "admin")
	{
		html_error ("Access denied.");
	}
}

function
redirect ($relative_url)
{
	global $HTTP_HOST, $PHP_SELF;
	header ("HTTP/1.1 303 See Other");
	header ("Location: http://$HTTP_HOST/" . dirname ($PHP_SELF) .
		"/$relative_url");
	exit ();
}


function
display_template_list ($selected_template)
{
        $res = db_query ("select template_id, template_name from playlist_template");
        echo "<option value=\"-1\">None Selected</option>\n";
	while ($row = mysql_fetch_assoc ($res)) {
        	$id = $row["template_id"];
        	$name = $row["template_name"];
        	echo "<option value=\"$id\"";
		if ($id == $selected_template)
			echo " selected";
		echo ">$name</option>\n";
        }
        mysql_free_result ($res);
}
?>