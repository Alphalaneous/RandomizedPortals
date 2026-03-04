#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJGameLevel.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/CheckpointObject.hpp>
#include <Geode/modify/PauseLayer.hpp>

using namespace geode::prelude;

static constexpr std::array<int, 5> s_speedPortals = {
    201, 200, 202, 203, 1334
};

static constexpr std::array<int, 5> s_speedPortalsNormal = {
    200, 201, 202, 203, 1334
};

void setupRandomSpeedsPre(GJBaseGameLayer* gjbgl, std::vector<int> speeds) {
    // only seems to work for particles that aren't preloaded
    int speedIdx = 1;
    for (auto obj : CCArrayExt<GameObject*>(gjbgl->m_objects)) {
        if (std::ranges::find(s_speedPortals, obj->m_objectID) != s_speedPortals.end()) {
            if (!obj->getUserObject("original-id"_spr)) {
                obj->setUserObject("original-id"_spr, CCInteger::create(obj->m_objectID));
            }

            if (speedIdx < speeds.size()) {
                auto speed = speeds[speedIdx];
                if (speed < 0 || speed > 4) {
                    obj->m_objectID = s_speedPortals[utils::random::generate(0, s_speedPortals.size() - 1)];
                }
                else {
                    obj->m_objectID = s_speedPortalsNormal[speeds[speedIdx]];
                }

                speedIdx++;
            }
            else {
                obj->m_objectID = s_speedPortals[utils::random::generate(0, s_speedPortals.size() - 1)];
            }

            std::string frame = ObjectToolbox::sharedState()->intKeyToFrame(obj->m_objectID);
            std::string particleFrameName = utils::string::replace(frame, "_001.png", "");
            std::string particleString = fmt::format("{}{}_effect.plist-22", obj->m_objectID, particleFrameName);
            obj->m_particleString = particleString;
        }
    }
}

void setupRandomSpeedsPost(GJBaseGameLayer* gjbgl) {
    // somewhat evil hack
    queueInMainThread([self = Ref(gjbgl)] {
        queueInMainThread([self] {
            for (auto obj : CCArrayExt<GameObject*>(self->m_objects)) {
                if (std::ranges::find(s_speedPortals, obj->m_objectID) != s_speedPortals.end()) {
                    std::string frame = ObjectToolbox::sharedState()->intKeyToFrame(obj->m_objectID);
                    CCSprite* spr = CCSprite::createWithSpriteFrameName(frame.c_str());
                    obj->setTextureRect(spr->getTextureRect(), spr->isTextureRectRotated(), spr->getTextureRect().size);
                    if (obj->m_glowSprite) {
                        std::string glowFrame = utils::string::replace(frame, "_001.png","_glow_001.png");
                        CCSprite* glowSpr = CCSprite::createWithSpriteFrameName(glowFrame.c_str());
                        obj->m_glowSprite->setTextureRect(glowSpr->getTextureRect(), glowSpr->isTextureRectRotated(), glowSpr->getTextureRect().size);
                    }
                    obj->updateOrientedBox();
                }
            }
        });
    });
}

class $modify(GJGameLevel) {
    void savePercentage(int p0, bool p1, int p2, int p3, bool p4) {
        if (Mod::get()->getSettingValue<bool>("soft-toggle") && Mod::get()->getSettingValue<bool>("safe-mode")) return;
        GJGameLevel::savePercentage(p0, p1, p2, p3, p4);
    }
};

class $modify(MyCheckpointObject, CheckpointObject) {
    struct Fields {
        Ref<EffectGameObject> m_speedObject;
    };
};

class $modify(MyPlayLayer, PlayLayer) {

    struct Fields {
        CCNodeRGBA* m_speedNode;
        Ref<EffectGameObject> m_lastSpeedObject;
    };

    void loadFromCheckpoint(CheckpointObject* object) {
        PlayLayer::loadFromCheckpoint(object);

        auto myCheckpointObject = static_cast<MyCheckpointObject*>(object);

        if (m_isPracticeMode && getCurrentPercent() > 0) {

            auto speedObj = myCheckpointObject->m_fields->m_speedObject;

            bool hasNoEffects = speedObj->m_hasNoEffects;
            speedObj->m_hasNoEffects = true;
            speedObj->triggerObject(this, 0, nullptr);
            speedObj->m_hasNoEffects = hasNoEffects;

            if (auto orig = typeinfo_cast<CCInteger*>(speedObj->getUserObject("original-id"_spr))) {
                updateSpeed(speedObj);
            }
        }
    }

    CheckpointObject* createCheckpoint() {
        auto ret = PlayLayer::createCheckpoint();
        static_cast<MyCheckpointObject*>(ret)->m_fields->m_speedObject = m_fields->m_lastSpeedObject;
        return ret;
    }

    void setupSpeedNode() {
        if (!Mod::get()->getSettingValue<bool>("soft-toggle")) return;
        auto fields = m_fields.self();
        if (!fields->m_speedNode) {
            fields->m_speedNode = CCNodeRGBA::create();
            fields->m_speedNode->setAnchorPoint({0.f, 1.f});
            fields->m_speedNode->setContentSize({60, 60});
            fields->m_speedNode->setScale(0.5f);
            fields->m_speedNode->setCascadeOpacityEnabled(true);

            auto winSize = CCDirector::get()->getWinSize();

            auto xOffset = Mod::get()->getSettingValue<float>("x-offset");
            auto yOffset = Mod::get()->getSettingValue<float>("y-offset");

            auto opacity = Mod::get()->getSettingValue<float>("opacity");

            fields->m_speedNode->setPosition({xOffset, winSize.height - yOffset});
            fields->m_speedNode->setOpacity(255 * opacity/100);

            m_uiLayer->addChild(fields->m_speedNode);
        }
    }

    void updateOpacity() {
        auto fields = m_fields.self();
        if (!fields->m_speedNode) return;
        
        auto opacity = Mod::get()->getSettingValue<float>("opacity");

        if (m_isPracticeMode) {
            opacity *= 0.5f;
        }
        fields->m_speedNode->setOpacity(255 * (opacity/100.f));
    }

    void togglePracticeMode(bool practiceMode) {
        PlayLayer::togglePracticeMode(practiceMode);
        if (Mod::get()->getSettingValue<bool>("soft-toggle")) {
            updateOpacity();
        }
    }

    void updateSpeed(GameObject* object) {
        setupSpeedNode();
        auto fields = m_fields.self();
        if (!fields->m_speedNode) return;

        fields->m_speedNode->removeAllChildren();
        
        int objectID;
        int originalID;

        if (!object) {
            float speed = m_player1->m_playerSpeed;
            
            if (speed == 0.7f) {
                objectID = 200;
            }
            else if (speed == 0.9f) {
                objectID = 201;
            }
            else if (speed == 1.1f) {
                objectID = 202;
            }
            else if (speed == 1.3f) {
                objectID = 203;
            }
            else if (speed == 1.6f) {
                objectID = 1334;
            }
        }
        else {
            objectID = object->m_objectID;
            if (auto orig = typeinfo_cast<CCInteger*>(object->getUserObject("original-id"_spr))) {
                originalID = orig->getValue();
            }
        }

        std::string frame = ObjectToolbox::sharedState()->intKeyToFrame(objectID);
        CCSprite* spr = CCSprite::createWithSpriteFrameName(frame.c_str());
        spr->setAnchorPoint({0.f, 0.f});

        std::string frameOrig = ObjectToolbox::sharedState()->intKeyToFrame(originalID);
        CCSprite* sprOrig = CCSprite::createWithSpriteFrameName(frameOrig.c_str());
        sprOrig->setScale(0.4f);
        sprOrig->setAnchorPoint({0.f, 0.f});

        sprOrig->setPositionX(spr->getContentWidth() + 5);
        fields->m_speedNode->addChild(sprOrig);

        fields->m_speedNode->addChild(spr);

        updateOpacity();
    }

    void showNewBest(bool p0, int p1, int p2, bool p3, bool p4, bool p5) {
        if (Mod::get()->getSettingValue<bool>("soft-toggle") && Mod::get()->getSettingValue<bool>("safe-mode")) return;
        PlayLayer::showNewBest(p0, p1, p2, p3, p4, p5);
    }

    void levelComplete() {
        if (Mod::get()->getSettingValue<bool>("soft-toggle") && Mod::get()->getSettingValue<bool>("safe-mode")) {
            bool testMode = m_isTestMode;
            m_isTestMode = true;
            PlayLayer::levelComplete();
            m_isTestMode = testMode;
            return;
        }
        PlayLayer::levelComplete();
    }

    void resetLevel() {
        PlayLayer::resetLevel();
    }

    static MyPlayLayer* get() {
        return static_cast<MyPlayLayer*>(PlayLayer::get());
    }
};

class $modify(MyGJBaseGameLayer, GJBaseGameLayer) {

    struct Fields {
        bool m_setAtStart = false;
        bool m_firstSetup = true;
        int m_startSpeed = -1;
        std::string m_speedStr;
        std::vector<int> m_speeds = {};
    };

    void resetRandom() {
        auto fields = m_fields.self();
        auto playLayer = PlayLayer::get();

        setupRandomSpeedsPre(this, fields->m_speeds);

        playLayer->resetLevel();

        setupRandomSpeedsPost(this);

        if (playLayer->getCurrentPercent() == 0) {
            fields->m_startSpeed = -1;
            setupRandomStartSpeed(fields->m_speeds);
        }
    }

    void parseSpeeds(const std::string& speeds) {
        auto fields = m_fields.self();
        fields->m_speeds.clear();
        fields->m_speedStr = speeds;
        auto speedsVec = utils::string::split(speeds, ",");
        for (auto speedStr : speedsVec) {
            if (auto speed = numFromString<int>(speedStr)) {
                fields->m_speeds.push_back(speed.unwrap());
            }
            else {
                fields->m_speeds.push_back(-1);
            }
        }
        resetRandom();
    }

    void setupRandomStartSpeed(std::vector<int> speeds) {
        auto fields = m_fields.self();

        if (fields->m_startSpeed == -1) {
            if (!speeds.empty()) {
                auto speed = speeds[0];
                if (speed == 0) speed = 1;
                else if (speed == 1) speed = 0;

                if (speed < 0 || speed > 4) {
                    fields->m_startSpeed = utils::random::generate(0, 4);
                }
                else {
                    fields->m_startSpeed = speed;
                }
            }
            else {
                fields->m_startSpeed = utils::random::generate(0, 4);
            }
        }

        auto speed = static_cast<EffectGameObject*>(GameObject::createWithKey(s_speedPortals[fields->m_startSpeed]));
        speed->m_hasNoEffects = true;
        speed->triggerObject(this, 0, nullptr);
        speed->setUserObject("original-id"_spr, CCInteger::create(s_speedPortals[static_cast<int>(m_levelSettings->m_startSpeed)]));

        if (auto playLayer = MyPlayLayer::get()) {
            playLayer->updateSpeed(speed);
        }
    }

    void setupLevelStart(LevelSettingsObject* p0) {
        auto playLayer = MyPlayLayer::get();
        if (!Mod::get()->getSettingValue<bool>("soft-toggle") || !playLayer) return GJBaseGameLayer::setupLevelStart(p0);

        auto fields = m_fields.self();

        if (fields->m_firstSetup) {
            fields->m_firstSetup = false;
            return GJBaseGameLayer::setupLevelStart(p0);
        }

        if (!fields->m_setAtStart) {
            fields->m_setAtStart = true;

            setupRandomSpeedsPre(this, fields->m_speeds);

            GJBaseGameLayer::setupLevelStart(p0);

            setupRandomSpeedsPost(this);
            setupRandomStartSpeed(fields->m_speeds);
        }
        else {
            GJBaseGameLayer::setupLevelStart(p0);
            if (playLayer->getCurrentPercent() == 0) {
                setupRandomStartSpeed(fields->m_speeds);
            }
        }
    }

    static MyGJBaseGameLayer* get() {
        if (PlayLayer::get()) return static_cast<MyGJBaseGameLayer*>(GJBaseGameLayer::get());
        return nullptr;
    }
};

struct SpeedMenu : public geode::Popup {

    static SpeedMenu* create() {
        auto ret = new SpeedMenu();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

    bool init() {
        if (!Popup::init({240.f, 130.f})) return false;
        setTitle("Set Speeds");

        m_input = geode::TextInput::create(200, "0,3,1,4");
        m_input->setFilter("-,0123456789");
        m_input->setMaxCharCount(0);

        if (auto gjbgl = MyGJBaseGameLayer::get()) {
            m_input->setString(gjbgl->m_fields->m_speedStr);
        }

        m_mainLayer->addChildAtPosition(m_input, Anchor::Center);

        auto spr = ButtonSprite::create("Apply");
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(SpeedMenu::onApply));

        btn->setPosition({m_buttonMenu->getContentWidth()/2.f, btn->getContentHeight()/2.f + 10.f});

        m_buttonMenu->addChild(btn);

        return true;
    }

    void onApply(CCObject* object) {
        if (auto gjbgl = MyGJBaseGameLayer::get()) {
            gjbgl->parseSpeeds(m_input->getString());
        }
    }

    geode::TextInput* m_input;
};

class $modify(MyPauseLayer, PauseLayer) {

    void customSetup() {
        PauseLayer::customSetup();
        if (auto menu = getChildByID("right-button-menu")) {

            auto spr = CircleButtonSprite::create(CCLabelBMFont::create("SP", "bigFont.fnt"));
            auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(MyPauseLayer::onSpeedMenu));

            menu->addChild(btn);
            menu->updateLayout();
        }
    }

    void onSpeedMenu(CCObject* object) {
        SpeedMenu::create()->show();
    }
};

$execute {
    listenForKeybindSettingPresses("reset-random", [](Keybind const& keybind, bool down, bool repeat, double timestamp) {
        if (!Mod::get()->getSettingValue<bool>("soft-toggle")) return;
        
        if (down && !repeat) {
            if (auto gjbgl = MyGJBaseGameLayer::get()) {
                gjbgl->resetRandom();
            }
        }
    });
}

class $modify(EffectGameObject) {

    void triggerObject(GJBaseGameLayer* p0, int p1, gd::vector<int> const* p2) {
        EffectGameObject::triggerObject(p0, p1, p2);

        auto playLayer = MyPlayLayer::get();
        if (!playLayer) return;

        if (Mod::get()->getSettingValue<bool>("soft-toggle")) {
            if (std::ranges::find(s_speedPortals, m_objectID) != s_speedPortals.end()) {
                playLayer->m_fields->m_lastSpeedObject = this;

                if (auto playLayer = static_cast<MyPlayLayer*>(PlayLayer::get())) {
                    playLayer->updateSpeed(this);
                }
            }
        }
    }
};
