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
db_connect ()
{
	global $DB_HOST, $DB_USER, $DB_PASSWORD, $station;
	mysql_connect ($DB_HOST, $DB_USER, $DB_PASSWORD)
		or html_error ("Could not open database connection.");
	if ($station)
		$db_name = "prs_$station";
	else
		$db_name = "prs";
	mysql_select_db ($db_name) or html_error ("Could not select database.");
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
?>