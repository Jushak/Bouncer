#include "BouncingScene.h"
#include "ui/CocosGUI.h"
#include "SimpleAudioEngine.h"

USING_NS_CC;

Scene* BouncingScene::createScene()
{
	auto scene = Scene::createWithPhysics();
	scene->getPhysicsWorld()->setGravity(Vec2(0, -98.0f));
	// Debug toggle.
	//scene->getPhysicsWorld()->setDebugDrawMask(0xffff);
	scene->getPhysicsWorld()->setSpeed(0);

	auto layer = BouncingScene::create();
	scene->addChild(layer);

	return scene;
}

bool BouncingScene::init()
{
	if (!Layer::init())
	{
		return false;
	}

	auto sprite = Sprite::create("grass_and_sky.jpg");
	sprite->setAnchorPoint(Vec2(0, 0));
	sprite->setScale(2.0);
	sprite->setPosition(0, 0);
	this->addChild(sprite, 0);

	// World boundaries.
	Size visibleSize = Director::getInstance()->getWinSize();
	Vec2 origin = Director::getInstance()->getVisibleOrigin();
	Size wallSize = Size(1, visibleSize.height * 3);
	Size groundSize = Size(visibleSize.width, 3);
	float adjustment = 20;

	// Left wall.
	auto leftWallNode = Node::create();
	auto leftWallBody = PhysicsBody::createEdgeBox(wallSize, PHYSICSBODY_MATERIAL_DEFAULT, 5);
	leftWallBody->setContactTestBitmask(0xFFFFFFFF);
	leftWallNode->setPosition(Point(adjustment, 0));
	leftWallNode->setPhysicsBody(leftWallBody);
	this->addChild(leftWallNode);

	// Right wall.
	auto rightWallNode = Node::create();
	auto rightWallBody = PhysicsBody::createEdgeBox(wallSize, PHYSICSBODY_MATERIAL_DEFAULT, 5);
	rightWallBody->setContactTestBitmask(0xFFFFFFFF);
	rightWallNode->setPosition(Point(visibleSize.width-adjustment, 0));
	rightWallNode->setPhysicsBody(rightWallBody);
	this->addChild(rightWallNode);

	// Ground.
	auto groundNode = Node::create();
	auto groundBody = PhysicsBody::createEdgeBox(groundSize, PHYSICSBODY_MATERIAL_DEFAULT, 5);
	groundBody->setContactTestBitmask(0xFFFFFFFF);
	groundNode->setTag(2);
	groundNode->setPosition(Point(visibleSize.width/2, 0));
	groundNode->setPhysicsBody(groundBody);
	this->addChild(groundNode);

	// Initialize base values.
	score = 0;
	scaleMulti = 1;
	circleSize = 12;
	force = 10000;

	// Add soccer ball to the scene..
	soccer_ball = Sprite::create("soccer_ball.png");
	soccer_ball->setPhysicsBody(PhysicsBody::createCircle(circleSize));
	soccer_ball->setPosition(visibleSize.width / 2, visibleSize.height);
	soccer_ball->setScale(scaleMulti);
	soccer_ball->getPhysicsBody()->setRotationEnable(false);
	soccer_ball->setTag(1);
	soccer_ball->getPhysicsBody()->setContactTestBitmask(0xFFFFFFFF);
	this->addChild(soccer_ball, 0);
	
	// Add contact listener for ending a game.
	auto contactListener = EventListenerPhysicsContact::create();
	contactListener->onContactBegin = CC_CALLBACK_1(BouncingScene::onContactBegin, this);
	_eventDispatcher->addEventListenerWithSceneGraphPriority(contactListener, this);

	// Add touch listeners to handle touch events.
	auto touchListener = EventListenerTouchOneByOne::create();
	touchListener->onTouchBegan = CC_CALLBACK_2(BouncingScene::onTouchBegan, this);
	touchListener->onTouchEnded = CC_CALLBACK_2(BouncingScene::onTouchEnded, this);
	touchListener->onTouchMoved = CC_CALLBACK_2(BouncingScene::onTouchMoved, this);
	touchListener->onTouchCancelled = CC_CALLBACK_2(BouncingScene::onTouchCancelled, this);
	_eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);

	// Add key listener for closing the application.
	auto keyListener = EventListenerKeyboard::create();
	keyListener->onKeyPressed = [](EventKeyboard::KeyCode keyCode, Event* event) {

		Vec2 loc = event->getCurrentTarget()->getPosition();
		if (keyCode == EventKeyboard::KeyCode::KEY_Q)
		{
			exit(0);
		}
	};
	_eventDispatcher->addEventListenerWithSceneGraphPriority(keyListener, this);
	
	// Add audio for kicking the ball.
	auto audio = CocosDenshion::SimpleAudioEngine::getInstance();
	audio->preloadEffect("ball_kick.wav");

	// Add score label
	score_label = Label::createWithSystemFont("", "Arial", 16);
	score_label->setPosition(visibleSize.width / 2, visibleSize.height * 3 / 4);
	score_label->setVisible(false);
	this->addChild(score_label);

	// Add start button that goes away when clicked.
	auto button = ui::Button::create("start_button.png", "start_button.png", "start_button.png");
	button->setTouchEnabled(true);
	button->setPosition(Point(visibleSize.width / 2, visibleSize.height / 2) + Point(0, -50));
	button->addTouchEventListener([&](Ref* sender, ui::Widget::TouchEventType type) {
		auto button = dynamic_cast<ui::Button * >(sender);
		switch (type)
		{
		case ui::Widget::TouchEventType::BEGAN:
			break;
		case ui::Widget::TouchEventType::ENDED:
			Director::getInstance()->getRunningScene()->getPhysicsWorld()->setSpeed(1);
			button->setVisible(false);
			button->setEnabled(false);
			button->cleanup();
			break;
		default:
			break;
		}
	});
	this->addChild(button);



	return true;
}

bool BouncingScene::onTouchBegan(Touch* touch, Event* event)
{
	//cocos2d::log("touch started");
	return true;
}

void BouncingScene::onTouchEnded(Touch* touch, Event* event)
{
	/*CCPoint location = touch->getLocationInView();
	location = CCDirector::sharedDirector()->convertToGL(location);*/

	auto location = touch->getLocation();

	if (soccer_ball->boundingBox().containsPoint(location))
	{
		// Play kick sound.
		auto audio = CocosDenshion::SimpleAudioEngine::getInstance();
		audio->playEffect("ball_kick.wav", false, 1.0f, 1.0f, 1.0f);
		
		// Score goes up.
		score++;

		// Stop the ball's downward movement before applying force to it.
		soccer_ball->getPhysicsBody()->setVelocity(Vec2(
			soccer_ball->getPhysicsBody()->getVelocity().x/2,0));
		auto ball_loc = soccer_ball->getPhysicsBody()->getPosition();
		//auto rotation = soccer_ball->getPhysicsBody()->getRotation();
		Point pBall = Point(ball_loc);
		Point pLoc = Point(location);
		Vec2 normalized = (ball_loc - location).getNormalized();
		float dist = pLoc.getDistance(pBall);
		float f_multi = dist / (scaleMulti*circleSize);
		Vec2 impulse = Vec2(force * f_multi * normalized.x, force * f_multi * normalized.y);
		soccer_ball->getPhysicsBody()->applyImpulse(impulse);
	}
}

void BouncingScene::onTouchMoved(Touch* touch, Event* event)
{
	//cocos2d::log("touch moved");
}

void BouncingScene::onTouchCancelled(Touch* touch, Event* event)
{
	//cocos2d::log("touch cancelled");
}

bool BouncingScene::onContactBegin(PhysicsContact& contact)
{
	// In case of multiple touch events are chained, ignore the ones after physics have been stopped.
	if (Director::getInstance()->getRunningScene()->getPhysicsWorld()->getSpeed() != 0)
	{
		auto nodeA = contact.getShapeA()->getBody()->getNode();
		auto nodeB = contact.getShapeB()->getBody()->getNode();

		// If the colliding objects are ball and ground, end game.
		if (nodeA->getTag() == 1 && nodeB->getTag() == 2
			|| nodeA->getTag() == 2 && nodeB->getTag() == 1)
		{
			cocos2d::log("Game end!");
			cocos2d::log("Final score: %d", score);
			
			// Show score.
			score_label->setString(StringUtils::format("Score: %d", score));
			score_label->setVisible(true);
			Size visibleSize = Director::getInstance()->getWinSize();
			
			// Stop physics.
			Director::getInstance()->getRunningScene()->getPhysicsWorld()->setSpeed(0);
			
			soccer_ball->getPhysicsBody()->setEnabled(false);

			// Add button for restaring the game.
			auto button = ui::Button::create("start_button.png", "start_button.png", "start_button.png");
			button->setTouchEnabled(true);
			button->setPosition(Point(visibleSize.width / 2, visibleSize.height / 2) + Point(0, -50));
		
			button->addTouchEventListener([&](Ref* sender, ui::Widget::TouchEventType type) {
				auto button = dynamic_cast<ui::Button * >(sender);
				Size visibleSize = Director::getInstance()->getWinSize();
				switch (type)
				{
				case ui::Widget::TouchEventType::BEGAN:
					break;
				case ui::Widget::TouchEventType::ENDED:
					// Reset score
					score = 0;
					// Reset ball position
					soccer_ball->setPosition(visibleSize.width / 2, visibleSize.height);
					soccer_ball->getPhysicsBody()->setVelocity(Vec2(0, 0));
					//soccer_ball->getPhysicsBody()->setAngularVelocity(0);
					//soccer_ball->setRotation(0);
					soccer_ball->getPhysicsBody()->setEnabled(true);
					Director::getInstance()->getRunningScene()->getPhysicsWorld()->setSpeed(1);
					score_label->setVisible(false);
					button->setVisible(false);
					button->setEnabled(false);
					button->cleanup();
					break;
				default:
					break;
				}
			});
			this->addChild(button);
		}
	}
	return true;
}