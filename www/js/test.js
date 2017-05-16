function createYoulostElement () {
	return $("<div id =\"you_lost\">Btw, <strong>you lost!</strong></div>");
}

function setElementPosition (node, position) {
	node.css({left: position.width  + "px"});
	node.css({top : position.height + "px"});
}

function generateRandomPagePosition () {
	let max_width =  $(document).width();
	let max_height = $(document).height();

	return {
		width : max_width  * Math.random(),
		height: max_height * Math.random()
	};
}

function randomlyMoveElement (node) {
	let random_position = generateRandomPagePosition();
	setElementPosition(node, random_position);
}

function createRandomlyMovingYouLostElement () {
	// Create the YouLost element, and append it to the body
	let you_lost_node = createYoulostElement();
	randomlyMoveElement(you_lost_node);

	$("body").append(you_lost_node);

	// Randomly change the node position every second
	setInterval(function () {
		randomlyMoveElement(you_lost_node);
	}, 1000);
}

$(document).ready(function () {
	$("#main_button").on("click", function () {
		createRandomlyMovingYouLostElement();
	});
});
