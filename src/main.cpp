#include "Geode/loader/Loader.hpp"
#include "Geode/loader/Log.hpp"
#include <Geode/Geode.hpp>
#include <Geode/binding/EffectGameObject.hpp>
#include <Geode/binding/GJBaseGameLayer.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJGameLevel.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <geode.custom-keybinds/include/Keybinds.hpp>
#include <random>

using namespace geode::prelude;
using namespace keybinds;

static constexpr std::array<int, 5> s_speedPortals = {
    201, 200, 202, 203, 1334
};

static int random(int min, int max) {
    static std::mt19937 rng{ std::random_device{}() };
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

void setupRandomSpeedsPre(GJBaseGameLayer* gjbgl) {
    // only seems to work for particles that aren't preloaded
    for (auto obj : CCArrayExt<GameObject*>(gjbgl->m_objects)) {
        if (std::ranges::find(s_speedPortals, obj->m_objectID) != s_speedPortals.end()) {
            if (!obj->getUserObject("original-id"_spr)) {
                obj->setUserObject("original-id"_spr, CCInteger::create(obj->m_objectID));
            }
            obj->m_objectID = s_speedPortals[random(0, s_speedPortals.size() - 1)];
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

class $modify(MyPlayLayer, PlayLayer) {

    struct Fields {
        CCNodeRGBA* m_speedNode;
    };

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

    void updateSpeed(GameObject* object, bool fromCheckpoint = false) {
        setupSpeedNode();
        auto fields = m_fields.self();
        if (!fields->m_speedNode) return;

        fields->m_speedNode->removeAllChildren();

        int objectID;
        int originalID;

        if (!object) {
            auto speed = m_player1->m_playerSpeed;
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

        if (!fromCheckpoint) {
            std::string frameOrig = ObjectToolbox::sharedState()->intKeyToFrame(originalID);
            CCSprite* sprOrig = CCSprite::createWithSpriteFrameName(frameOrig.c_str());
            sprOrig->setScale(0.4f);
            sprOrig->setAnchorPoint({0.f, 0.f});

            sprOrig->setPositionX(spr->getContentWidth() + 5);
            fields->m_speedNode->addChild(sprOrig);
        }

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
        if (Mod::get()->getSettingValue<bool>("soft-toggle")) {
            auto fields = m_fields.self();
            if (m_isPracticeMode && getCurrentPercent() > 0) {
                updateSpeed(nullptr, true);
            }
        }
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
    };

    void resetRandom() {
        auto fields = m_fields.self();
        auto playLayer = PlayLayer::get();

        setupRandomSpeedsPre(this);

        playLayer->resetLevel();

        setupRandomSpeedsPost(this);

        if (playLayer->getCurrentPercent() == 0) {
            fields->m_startSpeed = -1;
            setupRandomStartSpeed();
        }
    }

    void setupRandomStartSpeed() {
        auto fields = m_fields.self();

        if (fields->m_startSpeed == -1) {
            fields->m_startSpeed = random(0, 4);
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

            setupRandomSpeedsPre(this);

            GJBaseGameLayer::setupLevelStart(p0);

            setupRandomSpeedsPost(this);
            setupRandomStartSpeed();
        }
        else {
            GJBaseGameLayer::setupLevelStart(p0);
            if (playLayer->getCurrentPercent() == 0) {
                setupRandomStartSpeed();
            }
        }
    }

    static MyGJBaseGameLayer* get() {
        if (PlayLayer::get()) return static_cast<MyGJBaseGameLayer*>(GJBaseGameLayer::get());
        return nullptr;
    }
};

$execute {
    BindManager::get()->registerBindable({
        "reset-random"_spr,
        "Reset Random Portals",
        "",
        { Keybind::create(KEY_T, Modifier::None) },
        "",
        false
    });

    new EventListener(+[](InvokeBindEvent* event) {
        if (Mod::get()->getSettingValue<bool>("soft-toggle") && event->isDown()) {
            if (auto gjbgl = MyGJBaseGameLayer::get()) {
                gjbgl->resetRandom();
            }
        }
        return ListenerResult::Propagate;
    }, InvokeBindFilter(nullptr, "reset-random"_spr));

}

class $modify(EffectGameObject) {

    void triggerObject(GJBaseGameLayer* p0, int p1, gd::vector<int> const* p2) {
        EffectGameObject::triggerObject(p0, p1, p2);

        if (!PlayLayer::get()) return;

        if (Mod::get()->getSettingValue<bool>("soft-toggle")) {
            if (std::ranges::find(s_speedPortals, m_objectID) != s_speedPortals.end()) {
                if (auto playLayer = static_cast<MyPlayLayer*>(PlayLayer::get())) {
                    playLayer->updateSpeed(this);
                }
            }
        }
    }
};
