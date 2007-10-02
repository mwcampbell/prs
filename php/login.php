<?
require_once ("common.php");

function
startElement ($parser, $name, $attrs) {

	if ($name == "DB") {
// We can add to the session because it was started in common.php.
// We add these now so that db_connect() can use it in all circumstances.
		$_SESSION["DB_USER"] = $attrs["USER"];
		$_SESSION["DB_HOST"] = $attrs["HOST"];
		$_SESSION["DB_PASSWORD"] = $attrs["PASSWORD"];
		$_SESSION["DB_NAME"] = $attrs["NAME"];
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
		html_error ("Configuration file " . $filename . " not found.");
		exit ();
	}
	while ($data = fread ($fp, 4096)) {
		xml_parse ($parser, $data, feof($fp));
	}
}

parse_config_file ($_POST["station"]);

db_connect ();

if (!$_POST["username"] && !$_SESSION["user_username"])
	html_error ("Login error.");
else if (!$_SESSION["user_username"])
{
	$query = "select password, type from prs_user where user_name = '" . $_POST["username"] . "'";
	$res = db_query ($query);
	if ($row = mysql_fetch_assoc ($res))
	{
		$real_password = $row["password"];
		$real_type = $row["type"];
	}
	mysql_free_result ($res);
	if (!$real_password)
	{
		html_error ("there is no user by that name.");
	}
	else if ($real_password != $_POST["password"])
	{
		html_error ("Wrong password.");
	}
	else
	{
// We can assign to $_SESSION because session_start() was called in common.php
		$_SESSION["user_username"] = $_POST["username"];
		$_SESSION["user_password"] = $real_password;
		$_SESSION["user_type"] = $real_type;
		$_SESSION["station"] = $_POST["station"];
// DB session vars were set in startElement();
	}
}
redirect ("main.php");
?>
