<?
require_once ("common.php");
html_start ("Log In");
?>
<p>Welcome to the PRS Manager. Please log in.</p>

<form name="login" action="login.php" method="post">
<div><label for="username">Username:</label> <input type="text"
name="username" id="username"><br>
<label for="password">Password:</label> <input type="password"
name="password" id="password"><br>
<?
if ($STATIONS)
{
	echo ("<label for=\"station\">Station:</label>\n");
	echo ("<select id=\"station\" name=\"station\">\n");
	foreach ($STATIONS as $name => $label)
	{
		echo ("<option value=\"$name\">$label</option>\n");
	}
	echo ("</select><br>\n");
}
?>
<input type="submit" value="Log In"></div>
</form>
<?
html_end ();
?>
