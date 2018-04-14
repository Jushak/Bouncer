#include "BouncingScene.h"
#include "ui/CocosGUI.h"
#include "SimpleAudioEngine.h"

USING_NS_CC;

Scene* BouncingScene::createScene()
{
	auto scene = Scene::createWithPhysics();
	scene->getPhysicsWorld()->setGravity(Vec2(0, -98.0f));
	scene->getPhysicsWorld()->setDebugDrawMask(0xffff);
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

	// Initialize base values.
	score = 0;
	scaleMulti = 1;
	circleSize = 12;
	force = 10000;
	sound = true;
	debug = true;
	overlay = true;

	// Set background.
	auto sprite = Sprite::create("grass_and_sky.jpg");
	sprite->setAnchorPoint(Vec2(0, 0));
	sprite->setScale(2.0);
	sprite->setPosition(0, 0);
	this->addChild(sprite, 0);

	// Variables for setting scene boundaries.
	Size visibleSize = Director::getInstance()->getWinSize();
	Vec2 origin = Director::getInstance()->getVisibleOrigin();
	Size wallSize = Size(1, visibleSize.height * 3);
	Size groundSize = Size(visibleSize.width, 3);
	float adjustment = 20; // Window size is off, adjust for it.

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

	// Add soccer ball to the scene..
	soccer_ball = Sprite::create("soccer_ball.png");
	soccer_ball->setPhysicsBody(PhysicsBody::createCircle(circleSize));
	soccer_ball->setPosition(visibleSize.width / 2, visibleSize.height);
	soccer_ball->setScale(scaleMulti);
	soccer_ball->getPhysicsBody()->setRotationEnable(true);
	soccer_ball->setTag(1);
	soccer_ball->getPhysicsBody()->setContactTestBitmask(0xFFFFFFFF);
	soccer_ball->getPhysicsBody()->setGravityEnable(true);
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

	// Add keyboard listener to handle keyboard events.
	auto keyboardListener = EventListenerKeyboard::create();
	keyboardListener->onKeyPressed = CC_CALLBACK_2(BouncingScene::onKeyPressed, this);
	_eventDispatcher->addEventListenerWithSceneGraphPriority(keyboardListener, this);

	// Preload audio for kicking the ball.
	auto audio = CocosDenshion::SimpleAudioEngine::getInstance();
	audio->preloadEffect("ball_kick.wav");

	// Add score label.
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
	auto location = touch->getLocation();

	// If touch ended on the ball, process the touch. Otherwise ignore.
	if (soccer_ball->getBoundingBox().containsPoint(location))
	{
		// Play kick sound if sound is enabled.
		if (sound)
		{
			auto audio = CocosDenshion::SimpleAudioEngine::getInstance();
			audio->playEffect("ball_kick.wav", false, 1.0f, 1.0f, 1.0f);
		}
		
		// Increment score.
		score++;

		auto node_loc = soccer_ball->convertTouchToNodeSpaceAR(touch);
		float dist = node_loc.getDistance(Vec2(0, 0));
		float f_multi = dist / (scaleMulti * circleSize);
		auto ball_loc = soccer_ball->getPhysicsBody()->getPosition();
		Vec2 normalized = (Vec2(0, 0) -node_loc).getNormalized();
		auto offset = Vec2((ball_loc.x-location.x)/2, 0);
		Vec2 impulse = Vec2(force * f_multi * normalized.x, force * f_multi * normalized.y);
		soccer_ball->getPhysicsBody()->applyImpulse(impulse, offset);
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
					soccer_ball->getPhysicsBody()->setAngularVelocity(0);
					soccer_ball->setRotation(0);
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

void BouncingScene::onKeyPressed(EventKeyboard::KeyCode keyCode, Event*)
{
	auto scene = Director::getInstance()->getRunningScene();
	switch (keyCode)
	{
		case EventKeyboard::KeyCode::KEY_Q:
			exit(0);
			break;
		case EventKeyboard::KeyCode::KEY_S:
			sound = !sound;
			break;
		case EventKeyboard::KeyCode::KEY_D:
			debug = !debug;
			if (debug) {
				scene->getPhysicsWorld()->setDebugDrawMask(0xffff);
			}
			else {
				scene->getPhysicsWorld()->setDebugDrawMask(PhysicsWorld::DEBUGDRAW_NONE);
			}
			break;
		case EventKeyboard::KeyCode::KEY_G:
			overlay = !overlay;
			if (overlay) {
				Director::getInstance()->setDisplayStats(true);
			}
			else {
				Director::getInstance()->setDisplayStats(false);
			}
			break;
	}
}