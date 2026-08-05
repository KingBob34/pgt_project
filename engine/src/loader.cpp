#include "loader.h"
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace
{
    Condition parseCondition(const json& data)
    {
        Condition condition;
        condition.var = data.at("var").get<std::string>();
        condition.op = data.at("op").get<std::string>();
        condition.value = data.at("value").get<int>();
        return condition;
    }
}

Story loadStory(const std::string& path)
{
    std::ifstream file(path);
    if (!file)
    {
        throw std::runtime_error("Cannot open story file: " + path);
    }

    json data;
    try
    {
        data = json::parse(file);
    } catch (const json::parse_error& e)
    {
        throw std::runtime_error("Invalid JSON in " + path + " -- " + e.what());
    }

    Story story;
    story.title = data.at("title").get<std::string>();
    story.startId = data.at("start").get<std::string>();

    if (data.contains("initialVars"))
    {
        for (auto& [name, value] : data.at("initialVars").items())
        {
            story.initialVars[name] = value.get<int>();
        }
    }

    for (const json& passageData: data.at("passages"))
    {
        Passage passage;
        passage.id = passageData.at("id").get<std::string>();
        passage.text = passageData.at("text").get<std::string>();
        if (passageData.contains("type"))
        {
            passage.type = passageData.at("type").get<std::string>();
        }
        for (const json& choiceData: passageData.at("choices"))
        {
            Choice choice;
            choice.text = choiceData.at("text").get<std::string>();
            choice.target = choiceData.at("target").get<std::string>();
            if (choiceData.contains("condition"))
            {
                choice.hasCondition = true;
                choice.condition = parseCondition(choiceData.at("condition"));
            }
            if (choiceData.contains("whenUnmet"))
            {
                std::string mode = choiceData.at("whenUnmet").get<std::string>();
                if (mode == "show")
                {
                    choice.whenUnmet = ConUnmetDisplay::Show;
                } else
                {
                    choice.whenUnmet = ConUnmetDisplay::Hide;
                }
            }

            if (choiceData.contains("effects"))
            {
                for (const json& effectData : choiceData.at("effects"))
                {
                    Effect effect;
                    effect.var = effectData.at("var").get<std::string>();
                    effect.op = effectData.at("op").get<std::string>();
                    effect.value = effectData.at("value").get<int>();
                    choice.effects.push_back(effect);
                }
            }
            passage.choices.push_back(choice);
        }
        if (story.passages.count(passage.id) > 0)
        {
            throw std::runtime_error("Duplicate passage id: " + passage.id);
        }
        story.passages[passage.id] = passage;
    }

    if (story.passages.count(story.startId) == 0)
    {
        throw std::runtime_error("Start passage does not exist: " + story.startId);
    }

    for (const auto& [id, passage] : story.passages)
    {
        for (const Choice& choice : passage.choices)
        {
            if (story.passages.count(choice.target) == 0)
            {
                throw std::runtime_error(
                    "Passage \"" + id + "\" has a choice \"" + choice.text +
                    "\" pointing to a passage that does not exist: \"" + choice.target + "\""
                    );
            }
        }
    }
    return story;
}
