var SqlScript = function()
{
    return $('script[language=SQL]').text().trim();
}

var select = function(sql)
{
    if( ! (/^select/i).test(sql) ) return null;
    return /^select\s+(.*)\s+from\s+(.*)\s+where\s+(.*);/.exec(sql);
}

var SubmitForm = function()
{
    var matches = select(SqlScript());

    if(matches && matches.length > 2)
    {
        var jqstr = "$('" + matches[2] + " input[" + matches[3] + "]')";

        eval(jqstr).each(function()
        {
            var selector = 'dd#' + $(this)[0].id.replace("txt_", "show_");
            $(selector)[0].innerText = $(this)[0].value;
        });

        $('#whatever').slideDown();
    }
    else
    {
        console.log("There may be a problem with your SQL. No matches?");
    }

    return false;
}

// When Document is ready...
$(function() {
    $('#btn_submit').click(SubmitForm);
});
