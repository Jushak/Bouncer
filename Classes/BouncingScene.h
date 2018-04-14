#pragma once

#include "cocos2d.h"
#include "2d\CCLabel.h"

class BouncingScene : public cocos2d::Layer
{
public:
	static cocos2d::Scene* createScene();
	virtual bool init();

	virtual bool onTouchBegan(cocos2d::Touch*, cocos2d::Event*);
    virtual void onTouchEnded(cocos2d::Touch*, cocos2d::Event*);
    virtual void onTouchMoved(cocos2d::Touch*, cocos2d::Event*);
    virtual void onTouchCancelled(cocos2d::Touch*, cocos2d::Event*);
	virtual bool onContactBegin(cocos2d::PhysicsContact& contact);
	virtual void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event*);

	CREATE_FUNC(BouncingScene);
private:
	cocos2d::Sprite* soccer_ball;
	int score;
	float force;
	float scaleMulti;
	float circleSize;
	cocos2d::Label *score_label;
	bool sound;
	bool debug;
	bool overlay;
};

