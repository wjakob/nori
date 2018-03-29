!function ($) {
	$('a[href*=#]').on('click', function(event){
		$('html,body').animate({scrollTop:$(this.hash + "-header").offset().top}, 500);
	});

	prettify = function() {
		$("code").addClass("prettyprint");
		$("code").addClass("lang-cpp");
		prettyPrint();
		MathJax.Hub.Typeset();
	};

	$("#pa1-body").load("pa1.html", prettify);
	$("#pa2-body").load("pa2.html", prettify);
	$("#pa3-body").load("pa3.html", prettify);
	$("#pa4-body").load("pa4.html", prettify);
	$("#pa5-body").load("pa5.html", prettify);
	// $("#project-body").load("project.html", prettify);

	$(".fancybox").fancybox();

	$('.tooltips').tooltip({ selector: "span[rel=tooltip]" })
}(window.jQuery)

