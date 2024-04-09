#include <iostream>
#include <vector>
#include <random>
#include <ctime>
#include <SFML\Graphics.hpp>
#include <SFML\Audio.hpp>
#include <conio.h>
#include <windows.h>
#include <llama.h>
#include <common/common.h>
#include <common/console.h>
#include <ctime>
#include <filesystem>
using namespace std;

const int EMPTY = 0;
const int WALL = 1;
const int DOOR = 2;
const int PLAYER = 3;
const int EXIT = 4;

const int TILE_SIZE = 64;

class Maze {
private:
    sf::Texture textures[5];
    sf::Texture playerTextures[4];
    sf::Texture dialogueBoxTexture;
    sf::Texture doorTexture;
    sf::Texture thinkingDoorTexture;
    sf::Texture sweepyTexture;
    sf::Texture endScreenTexture;
    sf::Texture youLoseTexture;

public:
    int triesRemaining = 3;
    sf::SoundBuffer doorInteractBuffer;
    sf::Sound doorInteractSound;
    sf::SoundBuffer stepBuffer;
    sf::Sound stepSound;
    vector<string> professions1;
    vector<string> professions2;
    vector<string> professions3;
    vector<string> professions4;
    vector<string> professions5;
    sf::Texture startupScreenTexture;
    bool startupScreenDisplayed;
    bool isDoorThinking = false;
    bool playerLost = false;
    bool playerWon = false;
    bool everythingShown = false;
    sf::View gameView;
    sf::Vector2f gameOffset;
    llama_context_params ctx_params = llama_context_default_params();
    llama_context* ctx;
    llama_context* check_ctx;
    llama_sampling_context* ctx_sampling;
    llama_sampling_context* check_ctx_sampling;
    llama_model* model;
    int dialogueBoxState;
    char lastDirection;
    vector<vector<pair<int, pair<int, pair<string, pair<bool, bool>>>>>> grid;
    bool isDialogueBoxDisplayed;
    gpt_params params;
    int playerRow;
    int playerCol;
    string response;
    string check_response;
    sf::String playerInput;
    Maze() : playerRow(14), playerCol(0), lastDirection('s'), isDialogueBoxDisplayed(false), dialogueBoxState(1), startupScreenDisplayed(true) {
        grid.assign(15, vector<pair<int, pair<int, pair<string, pair<bool, bool>>>>>(15, { WALL, {0, {"", {false, false}}} }));
        // Initialize profession lists
        professions1 = { "Mathematician", "Historian", "Geographer", "Botanist", "Chemist", "Physicist", "Astronomer", "Zoologist", "Paleontologist", "Marine Biologist", "Nutritionist", "Geneticist", "Microbiologist", "Environmental Scientist", "Pharmacist", "Pathologist", "Civil Engineer", "Computer Scientist", "Mechanical Engineer" };
        professions2 = { "Biologist", "Economist", "Geologist", "Statistician", "Archaeologist", "Oceanographer", "Urban Planner", "Public Health Specialist", "Forensic Scientist", "Software Developer", "Electrical Engineer", "Materials Scientist", "Hydrologist", "Actuary", "Toxicologist", "Agricultural Scientist", "Educational Researcher", "Market Research Analyst", "Financial Analyst", "Industrial Engineer" };
        professions3 = { "Literary Critic", "Psychologist", "Sociologist", "Anthropologist", "Linguist", "Art Historian", "Musicologist", "Performance Artist", "Journalist", "Playwright", "Novelist", "Poet", "Screenwriter", "Critic (Art, Film)", "Historian of Science", "Cognitive Scientist", "Behavioral Economist", "Social Worker", "Cultural Studies Scholar", "Religious Studies Scholar" };
        // easy mode: professions4 = { "Philosopher", "Ethicist", "Theologian", "Cultural Critic", "Political Scientist", "Legal Scholar", "Political Analyst", "Diplomat", "Policy Maker", "Strategist (Military, Political)", "Civil Rights Activist", "Human Rights Lawyer", "International Relations Expert", "Social Philosopher", "Public Administrator", "Political Theorist" };
        // hard mode:
        professions4 = { "Entomologist", "Endocrinologist", "Epidemiologist", "Herpetologist", "Ichthyologist", "Malacologist", "Mycologist", "Ornithologist", "Primatologist", "Acarologist", "Bryologist", "Lichenologist", "Nematologist", "Palynologist", "Phycologist", "Protistologist", "Speleologist", "Volcanologist", "Cytogeneticist", "Histopathologist" };
        professions5 = { "String Theorist", "Politician", "Quantum Physicist", "Metaphysician", "Existentialist", "Cosmologist", "Theoretical Physicist", "Philosopher of Mind", "Logician" };
        generateMaze();
        grid[playerRow][playerCol].first = PLAYER;
        exploreAdjacentCells();
        loadTextures();
        llama_backend_init();
        llama_numa_init(params.numa);
        gameView.setSize(15 * TILE_SIZE, 15 * TILE_SIZE);
        gameView.setCenter(15 * TILE_SIZE / 2, 15 * TILE_SIZE / 2);
        gameView.setViewport(sf::FloatRect(0, 0, 1, 1));
        if (!doorInteractBuffer.loadFromFile("./sound/door_interact.wav")) {
            std::cout << "Error loading door_interact.wav" << std::endl;
        }
        doorInteractSound.setBuffer(doorInteractBuffer);
        if (!stepBuffer.loadFromFile("./sound/step.wav")) {
            std::cout << "Error loading step.wav" << std::endl;
        }
        stepSound.setBuffer(stepBuffer);
    }

    void loadTextures() {
        startupScreenTexture.loadFromFile("textures/startup_screen.png");
        textures[EMPTY].loadFromFile("textures/floorx64.png");
        textures[WALL].loadFromFile("textures/wallx64.png");
        textures[DOOR].loadFromFile("textures/doorx64.png");
        textures[EXIT].loadFromFile("textures/babax64.png");
        playerTextures[0].loadFromFile("textures/babie_floor_eastx64.png");
        playerTextures[1].loadFromFile("textures/babie_floor_southx64.png");
        playerTextures[2].loadFromFile("textures/babie_floor_westx64.png");
        playerTextures[3].loadFromFile("textures/babie_floor_northx64.png");
        dialogueBoxTexture.loadFromFile("textures/dialogue_box_large.png");
        doorTexture.loadFromFile("textures/door.png");
        thinkingDoorTexture.loadFromFile("textures/door_thinking.png");
        endScreenTexture.loadFromFile("textures/end_screen.png");
        youLoseTexture.loadFromFile("textures/you_lose.png");
    }

    void generateMaze() {
        generateEmptyLayer(14);
        for (int i = 4; i >= 0; i--) {
            generateDoorLayer(i * 3 + 1, 5 - i);
            generateBlockingLayer(i * 3);
            generateEmptyLayer(i * 3 + 2);
        }
        placeExit();
    }

    void generateDoorLayer(int row, int level) {
        int doorCount = (level == 5) ? 1 : 5;
        while (doorCount > 0) {
            int col = rand() % 15;
            if (grid[row][col].first != DOOR && (col == 0 || grid[row][col - 1].first != DOOR)) {
                string profession = getRandomProfession(level);
                bool isLiar = (rand() % 2 == 0);
                grid[row][col] = { DOOR, {level, {profession, {isLiar, false}}} };
                doorCount--;
            }
        }
    }

    void generateEmptyLayer(int row) {
        for (int col = 0; col < 15; col++) {
            grid[row][col] = { EMPTY, {0, {"", {false, false}}} };
        }
    }

    void generateBlockingLayer(int row) {
        bool allWalls = true;
        while (allWalls) {
            for (int col = 0; col < 15; col++) {
                if (grid[row + 1][col].first == DOOR) {
                    int unblockChance = rand() % 10;
                    if (unblockChance == 0) {
                        grid[row][col] = { EMPTY, {0, {"", {false, false}}} };
                    }
                }
            }

            // Check if the blocking layer is all walls
            allWalls = true;
            for (int col = 0; col < 15; col++) {
                if (grid[row][col].first != WALL) {
                    allWalls = false;
                    break;
                }
            }
        }
    }

    void placeExit() {
        for (int col = 0; col < 15; col++) {
            if (grid[1][col].first == DOOR && grid[1][col].second.first == 5) {
                grid[0][col] = { EXIT, {0, {"", {false, false}}} };
                break;
            }
        }
    }

    string getRandomProfession(int level) {
        vector<string>* professions = nullptr;
        switch (level) {
        case 1: professions = &professions1; break;
        case 2: professions = &professions2; break;
        case 3: professions = &professions3; break;
        case 4: professions = &professions4; break;
        case 5: professions = &professions5; break;
        default: return "Unknown";
        }

        if (professions->empty()) {
            return "Unknown";
        }

        int index = rand() % professions->size();
        string profession = (*professions)[index];
        professions->erase(professions->begin() + index);
        return profession;
    }

    std::string generateResponse(const std::string& profession, bool isLiar) {

        if (triesRemaining < 1) {
            return "You are out of tries!";
        }
        triesRemaining--;

        // Clean up
        check_ctx = llama_new_context_with_model(model, ctx_params);
        check_response.clear();
        ctx = llama_new_context_with_model(model, ctx_params);
        response.clear();

        // Check if the question is related to the profession
        std::string checkPrompt = "Instruction: Respond 'Yes' if the question: '" + playerInput + "' is generally related to the knowledge a " + profession + " would have, and is a question that is at least harder to answer than the question 'Are Zebras real'. If the question doesn't meet both these criteria, respond 'No'. Try to answer 'Yes' as often as possible.  Response: ";

        // Tokenize the check prompt
        std::vector<llama_token> check_tokens_list = ::llama_tokenize(check_ctx, checkPrompt, true);

        const int check_n_ctx = llama_n_ctx(check_ctx);
        const int check_n_len = 3; // Desired number of tokens to generate
        const int check_n_kv_req = check_tokens_list.size() + (check_n_len - check_tokens_list.size());

        if (check_n_kv_req > check_n_ctx) {
            std::cout << "Error: check_n_kv_req > check_n_ctx, the required KV cache size is not big enough" << std::endl;
            return "";
        }

        // Create a llama_batch for check with size 512
        llama_batch check_batch = llama_batch_init(512, 0, 1);

        // Evaluate the check prompt
        for (size_t i = 0; i < check_tokens_list.size(); i++) {
            llama_batch_add(check_batch, check_tokens_list[i], i, { 0 }, false);
        }

        check_batch.logits[check_batch.n_tokens - 1] = true;

        if (llama_decode(check_ctx, check_batch) != 0) {
            std::cout << "llama_decode() failed" << std::endl;
            return "";
        }

        // Main loop for question check
        int check_n_cur = 0;

        while (check_n_cur <= check_n_len) {
            // Sample the next token
            auto check_n_vocab = llama_n_vocab(model);
            auto* check_logits = llama_get_logits_ith(check_ctx, check_batch.n_tokens - 1);
            std::vector<llama_token_data> check_candidates;
            check_candidates.reserve(check_n_vocab);
            for (llama_token check_token_id = 0; check_token_id < check_n_vocab; check_token_id++) {
                check_candidates.emplace_back(llama_token_data{ check_token_id, check_logits[check_token_id], 0.0f });
            }
            llama_token_data_array check_candidates_p = { check_candidates.data(), check_candidates.size(), false };

            // Sample the most likely token
            const llama_token check_new_token_id = llama_sample_token_greedy(check_ctx, &check_candidates_p);

            // Is it an end of stream?
            if (check_new_token_id == llama_token_eos(model) || check_n_cur > check_n_len) {
                break;
            }

            check_response += llama_token_to_piece(check_ctx, check_new_token_id);

            // Prepare the next batch
            llama_batch_clear(check_batch);

            // Push this new token for next evaluation
            llama_batch_add(check_batch, check_new_token_id, check_n_cur, { 0 }, true);
            check_n_cur += 1;

            // Evaluate the current batch with the transformer model
            if (llama_decode(check_ctx, check_batch)) {
                std::cout << "Failed to eval, return code 1" << std::endl;
                return "";

            }
        }

        llama_batch_free(check_batch);

        //cout << "Check response: " << check_response << endl;

        if (check_response.find('y') == std::string::npos && check_response.find('Y') == std::string::npos) {
            return "This question is either too simple, or not related to the profession of " + profession + ", so the door was offended, and refused to answer! Try again!";
            
        }
        else {
            // Prepare the prompt
            std::string prompt = "Instruction: You are a " + profession + "." + std::string(isLiar ? "Please give an absolutely incorrect response to the question: '" : "Please give a factually correct response to the question: '") + playerInput + "' respond to this in one sentence only.  Response: ";

            // Tokenize the prompt
            std::vector<llama_token> tokens_list = ::llama_tokenize(ctx, prompt, true);

            const int n_ctx = llama_n_ctx(ctx);
            const int n_len = 100; // Desired number of tokens to generate
            const int n_kv_req = tokens_list.size() + (n_len - tokens_list.size());

            if (n_kv_req > n_ctx) {
                std::cout << "Error: n_kv_req > n_ctx, the required KV cache size is not big enough" << std::endl;
                return "";
            }

            // Create a llama_batch with size 512
            llama_batch batch = llama_batch_init(512, 0, 1);

            // Evaluate the initial prompt
            for (size_t i = 0; i < tokens_list.size(); i++) {
                llama_batch_add(batch, tokens_list[i], i, { 0 }, false);
            }

            batch.logits[batch.n_tokens - 1] = true;

            if (llama_decode(ctx, batch) != 0) {
                std::cout << "llama_decode() failed" << std::endl;
                return "";
            }

            // Main loop
            int n_cur = batch.n_tokens;

            while (n_cur <= n_len) {
                // Sample the next token
                auto n_vocab = llama_n_vocab(model);
                auto* logits = llama_get_logits_ith(ctx, batch.n_tokens - 1);
                std::vector<llama_token_data> candidates;
                candidates.reserve(n_vocab);
                for (llama_token token_id = 0; token_id < n_vocab; token_id++) {
                    candidates.emplace_back(llama_token_data{ token_id, logits[token_id], 0.0f });
                }
                llama_token_data_array candidates_p = { candidates.data(), candidates.size(), false };

                // Sample the most likely token
                const llama_token new_token_id = llama_sample_token_greedy(ctx, &candidates_p);

                // Is it an end of stream?
                if (new_token_id == llama_token_eos(model) || n_cur == n_len) {
                    break;
                }

                response += llama_token_to_piece(ctx, new_token_id);

                // Prepare the next batch
                llama_batch_clear(batch);

                // Push this new token for next evaluation
                llama_batch_add(batch, new_token_id, n_cur, { 0 }, true);
                n_cur += 1;

                // Evaluate the current batch with the transformer model
                if (llama_decode(ctx, batch)) {
                    std::cout << "Failed to eval, return code 1" << std::endl;
                    return "";
                }
            }

            llama_batch_free(batch);
            llama_free(ctx);
            llama_free(check_ctx);
            checkPrompt.clear();
            prompt.clear();
            playerInput.clear();

            response = "The door says: " + response;
            response.erase(std::remove(response.begin(), response.end(), '\n'), response.end());

            return response;
        }
    }

    void setDialogueBoxDisplayed(bool displayed) {
        isDialogueBoxDisplayed = displayed;
    }

    bool getDialogueBoxDisplayed() const {
        return isDialogueBoxDisplayed;
    }

    void displayDialogueBox(sf::RenderWindow& window, int doorRow, int doorCol, bool isLiar) {
        sf::Font font;
        sf::Sprite dialogueBoxSprite(dialogueBoxTexture);
        sf::FloatRect viewport = gameView.getViewport();
        sf::Vector2f viewSize = gameView.getSize();
        sf::Vector2f dialogueBoxSize(800, 420);

        sf::Vector2f dialogueBoxPosition;
        dialogueBoxPosition.x = (viewSize.x - dialogueBoxSize.x) / 2.0f;
        dialogueBoxPosition.y = viewSize.y - dialogueBoxSize.y;

        sf::Sprite doorSprite;
        if (isDoorThinking) {
            doorSprite.setTexture(thinkingDoorTexture);
        }
        else {
            doorSprite.setTexture(doorTexture);
        }
        // Scale the door sprite
        float doorScale = 0.5f; // Adjust this value to change the scale of the door sprite
        sf::Vector2f doorSpriteSize(doorSprite.getGlobalBounds().width * doorScale, doorSprite.getGlobalBounds().height * doorScale);
        doorSprite.setScale(doorSpriteSize.x / doorSprite.getGlobalBounds().width, doorSpriteSize.y / doorSprite.getGlobalBounds().height);

        sf::Vector2f doorSpritePosition;
        doorSpritePosition.x = (viewSize.x - doorSpriteSize.x) / 2.0f;
        doorSpritePosition.y = dialogueBoxPosition.y - doorSpriteSize.y;
        doorSprite.setPosition(doorSpritePosition);
        window.draw(doorSprite);

        dialogueBoxSprite.setPosition(dialogueBoxPosition);
        window.draw(dialogueBoxSprite);

        sf::Text dialogueText;
        font.loadFromFile("fonts/PAPYRUS.ttf");
        dialogueText.setFont(font);
        dialogueText.setCharacterSize(24);
        dialogueText.setFillColor(sf::Color::Black);
        dialogueText.setLineSpacing(1.2f);

        std::string profession = grid[doorRow][doorCol].second.second.first;
        std::string dialogue = "Hi, I'm a door, but I'm also a " + profession + ".\n Ask me up to 3 questions, to which I will either always lie, \n or always tell the truth. If you can guess if I always lie\n or tell the truth, I'll let you through!";

        if (dialogueBoxState == 1) {
            triesRemaining = 3;
            dialogueText.setString(dialogue);
            dialogueText.setPosition(dialogueBoxPosition.x + 100, dialogueBoxPosition.y + 125);
            window.draw(dialogueText);

            sf::Text tryText;
            tryText.setFont(font);
            tryText.setCharacterSize(24);
            tryText.setFillColor(sf::Color::Black);
            tryText.setString("Try");
            sf::Vector2f tryTextPosition;
            tryTextPosition.x = (viewSize.x - 200) / 2.0f;
            tryTextPosition.y = viewSize.y - 125;
            tryText.setPosition(tryTextPosition);
            window.draw(tryText);

            sf::Text leaveText;
            leaveText.setFont(font);
            leaveText.setCharacterSize(24);
            leaveText.setFillColor(sf::Color::Black);
            leaveText.setString("Leave");
            sf::Vector2f leaveTextPosition;
            leaveTextPosition.x = (viewSize.x + 100) / 2.0f;
            leaveTextPosition.y = viewSize.y - 125;
            leaveText.setPosition(leaveTextPosition);
            window.draw(leaveText);
        }
        else if (dialogueBoxState == 2) {
            sf::Text inputPrefix;
            inputPrefix.setFont(font);
            inputPrefix.setCharacterSize(24);
            inputPrefix.setFillColor(sf::Color::Black);
            inputPrefix.setString("You ask: ");
            sf::Vector2f inputPrefixPosition;
            inputPrefixPosition.x = dialogueBoxPosition.x + 100;
            inputPrefixPosition.y = dialogueBoxPosition.y + 145;
            inputPrefix.setPosition(inputPrefixPosition);
            window.draw(inputPrefix);

            // Wrap the player's input text
            unsigned int maxWidth = 550;
            std::string wrappedInput;
            std::vector<std::string> words;
            std::string currentWord;
            for (char c : playerInput) {
                if (c == ' ') {
                    if (!currentWord.empty()) {
                        words.push_back(currentWord);
                        currentWord.clear();
                    }
                }
                else {
                    currentWord += c;
                }
            }
            if (!currentWord.empty()) {
                words.push_back(currentWord);
            }

            sf::Text tempText;
            tempText.setFont(font);
            tempText.setCharacterSize(24);
            tempText.setString(""); // Clears the previous string content

            for (size_t i = 0; i < words.size(); i++) {
                std::string word = words[i];
                std::string potentialInput;
                if (wrappedInput.empty()) {
                    // Account for the "You ask: " text on the first line
                    potentialInput = "You ask: " + word;
                }
                else {
                    potentialInput = wrappedInput + (wrappedInput.empty() ? "" : " ") + word;
                }
                tempText.setString(potentialInput);
                if (tempText.getGlobalBounds().width > maxWidth) {
                    wrappedInput += "\n"; // Start a new line
                }
                else {
                    // Only add space if not starting a new line and wrappedInput is not empty
                    if (i > 0) {
                        wrappedInput += " ";
                    }
                }
                wrappedInput += word;
            }

            sf::Text questionText;
            questionText.setFont(font);
            questionText.setCharacterSize(24);
            questionText.setFillColor(sf::Color::Black);
            questionText.setString("                   " + wrappedInput);
            sf::Vector2f questionTextPosition;
            questionTextPosition.x = dialogueBoxPosition.x + 100;
            questionTextPosition.y = dialogueBoxPosition.y + 145;
            questionText.setPosition(questionTextPosition);
            window.draw(questionText);
        }
        else if (dialogueBoxState == 3) {

            sf::Text responseText;
            responseText.setFont(font);
            //responseText.setCharacterSize(24);
            //responseText.setFillColor(sf::Color::Black);
            //responseText.setString(response);
            //responseText.setPosition(gameOffset.x + (15 * TILE_SIZE - 780) / 2 + 125, gameOffset.y + 15 * TILE_SIZE - 300);
            // window.draw(responseText);
            displayResponseText(window, font);

            sf::Text askButton;
            askButton.setFont(font);
            askButton.setCharacterSize(24);
            askButton.setFillColor(sf::Color::Black);
            askButton.setString("Ask again");
            sf::Vector2f askButtonPosition;
            askButtonPosition.x = dialogueBoxPosition.x + 150;
            askButtonPosition.y = dialogueBoxPosition.y + 270;
            askButton.setPosition(askButtonPosition);
            window.draw(askButton);

            sf::Text guessButton;
            guessButton.setFont(font);
            guessButton.setCharacterSize(24);
            guessButton.setFillColor(sf::Color::Black);
            guessButton.setString("Guess truthfulness");
            sf::Vector2f guessButtonPosition;
            guessButtonPosition.x = dialogueBoxPosition.x + 300;
            guessButtonPosition.y = dialogueBoxPosition.y + 270;
            guessButton.setPosition(guessButtonPosition);
            window.draw(guessButton);

            sf::Text giveUpButton;
            giveUpButton.setFont(font);
            giveUpButton.setCharacterSize(24);
            giveUpButton.setFillColor(sf::Color::Black);
            giveUpButton.setString("Give up");
            sf::Vector2f giveUpButtonPosition;
            giveUpButtonPosition.x = dialogueBoxPosition.x + 550;
            giveUpButtonPosition.y = dialogueBoxPosition.y + 270;
            giveUpButton.setPosition(giveUpButtonPosition);
            window.draw(giveUpButton);
        }
        else if (dialogueBoxState == 4) {
            sf::Text truthfulnessPrompt;
            truthfulnessPrompt.setFont(font);
            truthfulnessPrompt.setCharacterSize(24);
            truthfulnessPrompt.setFillColor(sf::Color::Black);
            truthfulnessPrompt.setString("Is the door a Liar or Honest?");
            sf::Vector2f truthfulnessPromptPosition;
            truthfulnessPromptPosition.x = (viewSize.x - 300) / 2.0f;
            truthfulnessPromptPosition.y = dialogueBoxPosition.y + 120;
            truthfulnessPrompt.setPosition(truthfulnessPromptPosition);
            window.draw(truthfulnessPrompt);

            sf::Text liarButton;
            liarButton.setFont(font);
            liarButton.setCharacterSize(24);
            liarButton.setFillColor(sf::Color::Black);
            liarButton.setString("Liar");
            sf::Vector2f liarButtonPosition;
            liarButtonPosition.x = (viewSize.x + 100) / 2.0f;
            liarButtonPosition.y = dialogueBoxPosition.y + 270;
            liarButton.setPosition(liarButtonPosition);
            window.draw(liarButton);

            sf::Text honestButton;
            honestButton.setFont(font);
            honestButton.setCharacterSize(24);
            honestButton.setFillColor(sf::Color::Black);
            honestButton.setString("Honest");
            sf::Vector2f honestButtonPosition;
            honestButtonPosition.x = (viewSize.x - 250) / 2.0f;
            honestButtonPosition.y = dialogueBoxPosition.y + 270;
            honestButton.setPosition(honestButtonPosition);
            window.draw(honestButton);
        }

        else if (dialogueBoxState == 5) {
            sf::Sprite endScreenSprite(endScreenTexture);
            sf::Vector2f endScreenPosition;

            // Scale the win screen sprite
            float endScreenScale = 0.5f; // Adjust this value to change the scale of the win screen sprite
            sf::Vector2f endScreenSpriteSize(endScreenSprite.getGlobalBounds().width* endScreenScale, endScreenSprite.getGlobalBounds().height* endScreenScale);
            endScreenSprite.setScale(endScreenSpriteSize.x / endScreenSprite.getGlobalBounds().width, endScreenSpriteSize.y / endScreenSprite.getGlobalBounds().height);

            sf::Vector2f endScreenSpritePosition;
            endScreenSpritePosition.x = (viewSize.x - endScreenSpriteSize.x) / 2.0f;
            endScreenSpritePosition.y = dialogueBoxPosition.y - endScreenSpriteSize.y;
            endScreenSprite.setPosition(endScreenSpritePosition);

            window.draw(endScreenSprite);

            sf::Text winText;
            winText.setFont(font);
            winText.setCharacterSize(24);
            winText.setFillColor(sf::Color::Black);
            winText.setString("Baba acquired, nap accomplished, you win!");
            sf::Vector2f winTextPosition;
            winTextPosition.x = (viewSize.x - winText.getGlobalBounds().width) / 2.0f;
            winTextPosition.y = dialogueBoxPosition.y + 120;
            winText.setPosition(winTextPosition);
            window.draw(winText);

            sf::Text playAgainButton;
            playAgainButton.setFont(font);
            playAgainButton.setCharacterSize(24);
            playAgainButton.setFillColor(sf::Color::Black);
            playAgainButton.setString("Play Again");
            sf::Vector2f playAgainButtonPosition;
            playAgainButtonPosition.x = (viewSize.x - 250) / 2.0f;
            playAgainButtonPosition.y = dialogueBoxPosition.y + 270;
            playAgainButton.setPosition(playAgainButtonPosition);
            window.draw(playAgainButton);

            sf::Text exitButton;
            exitButton.setFont(font);
            exitButton.setCharacterSize(24);
            exitButton.setFillColor(sf::Color::Black);
            exitButton.setString("Exit");
            sf::Vector2f exitButtonPosition;
            exitButtonPosition.x = (viewSize.x + 100) / 2.0f;
            exitButtonPosition.y = dialogueBoxPosition.y + 270;
            exitButton.setPosition(exitButtonPosition);
            window.draw(exitButton);
            }
        else if (dialogueBoxState == 6) {
            sf::Sprite failureScreenSprite(youLoseTexture);
            sf::Vector2f failureScreenPosition;

            // Scale the failure screen sprite
            float failureScreenScale = 0.5f;
            sf::Vector2f failureScreenSpriteSize(failureScreenSprite.getGlobalBounds().width * failureScreenScale, failureScreenSprite.getGlobalBounds().height * failureScreenScale);
            failureScreenSprite.setScale(failureScreenSpriteSize.x / failureScreenSprite.getGlobalBounds().width, failureScreenSpriteSize.y / failureScreenSprite.getGlobalBounds().height);

            sf::Vector2f failureScreenSpritePosition;
            failureScreenSpritePosition.x = (viewSize.x - failureScreenSpriteSize.x) / 2.0f;
            failureScreenSpritePosition.y = dialogueBoxPosition.y - failureScreenSpriteSize.y;
            failureScreenSprite.setPosition(failureScreenSpritePosition);

            window.draw(failureScreenSprite);

            sf::Text loseText;
            loseText.setFont(font);
            loseText.setCharacterSize(24);
            loseText.setFillColor(sf::Color::Black);
            loseText.setString("The baba is unreachable, ultimate despair! You lose!");
            sf::Vector2f loseTextPosition;
            loseTextPosition.x = (viewSize.x - loseText.getGlobalBounds().width) / 2.0f;
            loseTextPosition.y = dialogueBoxPosition.y + 120;
            loseText.setPosition(loseTextPosition);
            window.draw(loseText);

            sf::Text playAgainButton;
            playAgainButton.setFont(font);
            playAgainButton.setCharacterSize(24);
            playAgainButton.setFillColor(sf::Color::Black);
            playAgainButton.setString("Play Again");
            sf::Vector2f playAgainButtonPosition;
            playAgainButtonPosition.x = (viewSize.x - 250) / 2.0f;
            playAgainButtonPosition.y = dialogueBoxPosition.y + 270;
            playAgainButton.setPosition(playAgainButtonPosition);
            window.draw(playAgainButton);

            sf::Text exitButton;
            exitButton.setFont(font);
            exitButton.setCharacterSize(24);
            exitButton.setFillColor(sf::Color::Black);
            exitButton.setString("Exit");
            sf::Vector2f exitButtonPosition;
            exitButtonPosition.x = (viewSize.x + 100) / 2.0f;
            exitButtonPosition.y = dialogueBoxPosition.y + 270;
            exitButton.setPosition(exitButtonPosition);
            window.draw(exitButton);
            }
        }

    void displayResponseText(sf::RenderWindow& window, const sf::Font& font) {
        sf::Text responseText;
        responseText.setFont(font);
        responseText.setCharacterSize(24);
        responseText.setFillColor(sf::Color::Black);
        responseText.setPosition(gameOffset.x + (15 * TILE_SIZE - 780) / 2 + 125, gameOffset.y + 15 * TILE_SIZE - 300);

        // Generate the complete response without displaying it character by character
        responseText.setString(response);

        // Wrap the response text if it exceeds the maximum width
        unsigned int maxWidth = 575;
        std::string wrappedResponse;
        std::vector<std::string> words;
        std::string currentWord;
        for (char c : response) {
            if (c == ' ') {
                if (!currentWord.empty()) {
                    words.push_back(currentWord);
                    currentWord.clear();
                }
            }
            else {
                currentWord += c;
            }
        }
        if (!currentWord.empty()) {
            words.push_back(currentWord);
        }

        sf::Text tempText;
        tempText.setFont(font);
        tempText.setCharacterSize(24);
        tempText.setString(""); // Clears the previous string content

        for (const std::string& word : words) {
            // Only add the space if wrappedResponse is not empty to avoid leading spaces
            std::string potentialResponse = wrappedResponse + (wrappedResponse.empty() ? "" : " ") + word;
            tempText.setString(potentialResponse);
            if (tempText.getGlobalBounds().width > maxWidth) {
                wrappedResponse += "\n"; // Start a new line
            }
            else {
                // Only add space if not starting a new line and wrappedResponse is not empty
                if (!wrappedResponse.empty()) {
                    wrappedResponse += " ";
                }
            }
            wrappedResponse += word;
        }


        responseText.setString(wrappedResponse);

        // Display the complete response
        window.draw(responseText);
    }

    void setGameOffset(const sf::Vector2f& offset) {
        gameOffset = offset;
    }

    void display(sf::RenderWindow& window) {
        window.setView(gameView);

        for (int row = 0; row < 15; row++) {
            for (int col = 0; col < 15; col++) {
                if (grid[row][col].second.second.second.second) {
                    sf::Sprite sprite(textures[grid[row][col].first]);
                    sprite.setPosition(col * TILE_SIZE, row * TILE_SIZE);
                    window.draw(sprite);

                    if (grid[row][col].first == PLAYER) {
                        sf::Sprite playerSprite;
                        if (lastDirection == 'd')
                            playerSprite.setTexture(playerTextures[0]);
                        else if (lastDirection == 's')
                            playerSprite.setTexture(playerTextures[1]);
                        else if (lastDirection == 'a')
                            playerSprite.setTexture(playerTextures[2]);
                        else if (lastDirection == 'w')
                            playerSprite.setTexture(playerTextures[3]);
                        playerSprite.setPosition(col * TILE_SIZE, row * TILE_SIZE);
                        window.draw(playerSprite);
                    }
                }
            }
        }

        if (isDialogueBoxDisplayed) {
            int doorRow = playerRow, doorCol = playerCol;
            switch (lastDirection) {
            case 'w': doorRow--; break;
            case 's': doorRow++; break;
            case 'd': doorCol++; break;
            case 'a': doorCol--; break;
            }
            bool isLiar = grid[doorRow][doorCol].second.second.second.first;
            if (dialogueBoxState != 5 && dialogueBoxState != 6) {
                displayDialogueBox(window, doorRow, doorCol, isLiar);
            }
            else {
                displayDialogueBox(window, 0, 0, false);
            }
        }

        window.setView(window.getDefaultView());
    }

    sf::Vector2f getDialogueBoxPosition() const {
        sf::FloatRect viewport = gameView.getViewport();
        sf::Vector2f viewSize = gameView.getSize();
        sf::Vector2f dialogueBoxSize(800, 420);

        sf::Vector2f dialogueBoxPosition;
        dialogueBoxPosition.x = viewport.left + (viewSize.x - dialogueBoxSize.x) / 2.0f;
        dialogueBoxPosition.y = viewport.top + viewSize.y - dialogueBoxSize.y;

        return dialogueBoxPosition;
    }


    void movePlayer(char direction, sf::RenderWindow& window) {
        lastDirection = direction;
        int newRow = playerRow, newCol = playerCol;
        switch (direction) {
        case 'w': newRow--; break;
        case 's': newRow++; break;
        case 'd': newCol++; break;
        case 'a': newCol--; break;
        }

        if (newRow >= 0 && newRow < 15 && newCol >= 0 && newCol < 15) {
            if (grid[newRow][newCol].first == DOOR) {
                isDialogueBoxDisplayed = true;
                doorInteractSound.play();
                return;
            }
            if (grid[newRow][newCol].first == EMPTY || grid[newRow][newCol].first == EXIT) {
                stepSound.play();
                grid[playerRow][playerCol].first = EMPTY;
                playerRow = newRow;
                playerCol = newCol;
                grid[playerRow][playerCol] = { PLAYER, {0, {"", {false, false}}} };
                exploreAdjacentCells();

                if (playerRow == 0) {
                    playerWon = true;
                    if (everythingShown == false) {
                        exploreAdjacentCells();
                        everythingShown == true;
                    }
                    showEndScreen(window);
                }
            }
            else if (grid[newRow][newCol].first == WALL) {
                //cout << "You cannot move through a wall!" << endl;
                Sleep(100);
            }
        }
        else {
            //cout << "You cannot move outside the maze boundaries!" << endl;
            Sleep(100);
        }
    }

    bool isAdvancementImpossible() {
        if (playerRow <= 1) {
            return false;
        }

        vector<int> row1(15, 0);
        vector<int> row2(15, 0);
        vector<int> combinedRow(15, 0);

        for (int col = 0; col < 15; col++) {
            if (grid[playerRow - 1][col].first == WALL) {
                row1[col] = 1;
            }
            if (grid[playerRow - 2][col].first == WALL) {
                row2[col] = 1;
            }
        }

        for (int col = 0; col < 15; col++) {
            if (row1[col] == 1 || row2[col] == 1) {
                combinedRow[col] = 1;
            }
        }

        return all_of(combinedRow.begin(), combinedRow.end(), [](int i) { return i == 1; });
    }

    void showEndScreen(sf::RenderWindow& window) {
        // Change player representation to sweepy.png
        playerTextures[0].loadFromFile("textures/sweepyx64.png");
        playerTextures[1].loadFromFile("textures/sweepyx64.png");
        playerTextures[2].loadFromFile("textures/sweepyx64.png");
        playerTextures[3].loadFromFile("textures/sweepyx64.png");

        display(window);
        window.display();

        // Lock player input
        sf::sleep(sf::seconds(3));

        // Display end screen
        dialogueBoxState = 5;
        isDialogueBoxDisplayed = true;
    }

    void showFailureScreen(sf::RenderWindow& window) {

        display(window);
        window.display();

        sf::sleep(sf::seconds(2));

        dialogueBoxState = 6;
        isDialogueBoxDisplayed = true;
    }

    void exploreAdjacentCells() {
        for (int row = max(0, playerRow - 1); row <= min(14, playerRow + 1); row++) {
            for (int col = max(0, playerCol - 1); col <= min(14, playerCol + 1); col++) {
                grid[row][col].second.second.second.second = true;
            }
        }

        if (playerLost || playerWon) {
            for (int row = 0; row < 15; row++) {
                for (int col = 0; col < 15; col++) {
                    grid[row][col].second.second.second.second = true;
                }
            }
        }
    }
};


int main() {
    sf::RenderWindow window(sf::VideoMode(15 * TILE_SIZE, 15 * TILE_SIZE), "Lucy's Labyrinth");
    window.setFramerateLimit(60);
    window.setVerticalSyncEnabled(true);
    window.create(sf::VideoMode::getFullscreenModes()[0], "Lucy's Labyrinth", sf::Style::Fullscreen);
    sf::Music music;
    if (!music.openFromFile("./sound/music.wav")) {
        return 1;
    }
    music.setLoop(true);
    sf::SoundBuffer doorFailedBuffer;
    sf::SoundBuffer doorLeavingBuffer;
    sf::SoundBuffer doorOpenBuffer;
    sf::SoundBuffer winBuffer;
    sf::SoundBuffer loseBuffer;

    sf::Sound doorFailedSound;
    sf::Sound doorLeavingSound;
    sf::Sound doorOpenSound;
    sf::Sound winSound;
    sf::Sound loseSound;

    if (!doorFailedBuffer.loadFromFile("./sound/door_failed.wav")) {
        std::cout << "Error loading door_failed.wav" << std::endl;
    }
    doorFailedSound.setBuffer(doorFailedBuffer);

    if (!doorLeavingBuffer.loadFromFile("./sound/door_leaving.wav")) {
        std::cout << "Error loading door_leaving.wav" << std::endl;
    }
    doorLeavingSound.setBuffer(doorLeavingBuffer);

    if (!doorOpenBuffer.loadFromFile("./sound/door_open.wav")) {
        std::cout << "Error loading door_open.wav" << std::endl;
    }
    doorOpenSound.setBuffer(doorOpenBuffer);

    if (!winBuffer.loadFromFile("./sound/win.wav")) {
        std::cout << "Error loading win.wav" << std::endl;
    }
    winSound.setBuffer(winBuffer);

    if (!loseBuffer.loadFromFile("./sound/lose.wav")) {
        std::cout << "Error loading lose.wav" << std::endl;
    }
    loseSound.setBuffer(loseBuffer);


    srand(static_cast<unsigned int>(std::time(0)));
    Maze maze;
    bool playerGuess{};
    bool clickInsideButton = false;

    // Initialize the model
    llama_model_params model_params = llama_model_default_params();
    // Search for the first .gguf file in the "model" directory
    std::string modelPath;
    for (const auto& entry : std::filesystem::directory_iterator("./model")) {
        if (entry.path().extension() == ".gguf") {
            modelPath = entry.path().string();
            break;
        }
    }

    if (modelPath.empty()) {
        std::cout << "Error: no .gguf model file found in the 'model' directory" << std::endl;
        return 1;
    }

    // Load the model
    maze.model = llama_load_model_from_file(modelPath.c_str(), model_params);
    if (maze.model == NULL) {
        std::cout << "Error: unable to load model" << std::endl;
        return 1;
    }
    if (maze.model == NULL) {
        std::cout << "Error: unable to load model" << std::endl;
        return 1;
    }

    // Initialize the primary context
    maze.ctx_params.seed = 1234;
    maze.ctx_params.n_ctx = 2048;
    maze.ctx_params.n_threads = maze.params.n_threads;
    maze.ctx_params.n_threads_batch = maze.params.n_threads_batch == -1 ? maze.params.n_threads : maze.params.n_threads_batch;
    maze.ctx = llama_new_context_with_model(maze.model, maze.ctx_params);
    maze.check_ctx = llama_new_context_with_model(maze.model, maze.ctx_params);
    if (maze.ctx == NULL) {
        std::cout << "Error: failed to create the llama_context" << std::endl;
        return 1;
    }
    if (maze.check_ctx == NULL) {
        std::cout << "Error: failed to create the llama_context" << std::endl;
        return 1;
    }

    sf::Font font;
    font.loadFromFile("fonts/PAPYRUS.ttf");

    music.play();

    while (window.isOpen()) {

        sf::Vector2f dialogueBoxPosition = maze.getDialogueBoxPosition();
        sf::Vector2f viewSize = maze.gameView.getSize();

        sf::Vector2u windowSize = window.getSize();
        float windowRatio = static_cast<float>(windowSize.x) / windowSize.y;
        float viewRatio = static_cast<float>(maze.gameView.getSize().x) / maze.gameView.getSize().y;

        if (windowRatio >= viewRatio) {
            float viewHeight = windowSize.y;
            float viewWidth = viewHeight * viewRatio;
            float viewX = (windowSize.x - viewWidth) / 2.0f;
            maze.gameView.setViewport(sf::FloatRect(viewX / windowSize.x, 0, viewWidth / windowSize.x, 1));
        }
        else {
            float viewWidth = windowSize.x;
            float viewHeight = viewWidth / viewRatio;
            float viewY = (windowSize.y - viewHeight) / 2.0f;
            maze.gameView.setViewport(sf::FloatRect(0, viewY / windowSize.y, 1, viewHeight / windowSize.y));
        }

        if (music.getStatus() != sf::Music::Playing && ((maze.dialogueBoxState != 5 && maze.dialogueBoxState != 6) || maze.getDialogueBoxDisplayed() == false)) {
            music.play();
        }
        else if ((maze.dialogueBoxState == 5 || maze.dialogueBoxState == 6) && music.getStatus() == sf::Music::Playing) {
            music.stop();
            if (maze.dialogueBoxState == 5) {
                winSound.play();
            }
            else if (maze.dialogueBoxState == 6) {
                loseSound.play();
            }
        }

        int doorRow = maze.playerRow, doorCol = maze.playerCol;
        switch (maze.lastDirection) {
        case 'w': doorRow--; break;
        case 's': doorRow++; break;
        case 'd': doorCol++; break;
        case 'a': doorCol--; break;
        }

        if (maze.isAdvancementImpossible()) {
            maze.playerLost = true;
            if (maze.everythingShown == false) {
                maze.exploreAdjacentCells();
                maze.everythingShown == true;
            }
            maze.showFailureScreen(window);
        }
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            else if (event.type == sf::Event::KeyPressed && maze.startupScreenDisplayed) {
                maze.startupScreenDisplayed = false;
            }
            else if (event.type == sf::Event::MouseButtonPressed) {
                if (maze.getDialogueBoxDisplayed()) {
                    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                    sf::Vector2f mousePosF = window.mapPixelToCoords(mousePos, maze.gameView);

                    if (maze.dialogueBoxState == 1) {
                        sf::FloatRect tryBounds((viewSize.x - 200) / 2.0f, viewSize.y - 125, 50, 30);
                        sf::FloatRect leaveBounds((viewSize.x + 100) / 2.0f, viewSize.y - 125, 70, 30);

                        if (tryBounds.contains(mousePosF.x, mousePosF.y)) {
                            maze.dialogueBoxState = 2;
                        }
                        else if (leaveBounds.contains(mousePosF.x, mousePosF.y)) {
                            doorLeavingSound.play();
                            maze.isDialogueBoxDisplayed = false;
                        }
                    }
                    else if (maze.dialogueBoxState == 3) {
                        sf::FloatRect askBounds(dialogueBoxPosition.x + 150, dialogueBoxPosition.y + 270, 100, 30);
                        sf::FloatRect guessBounds(dialogueBoxPosition.x + 300, dialogueBoxPosition.y + 270, 200, 30);
                        sf::FloatRect giveUpBounds(dialogueBoxPosition.x + 550, dialogueBoxPosition.y + 270, 100, 30);

                        if (askBounds.contains(mousePosF.x, mousePosF.y)) {
                            maze.dialogueBoxState = 2;
                            maze.playerInput.clear();
                        }
                        else if (guessBounds.contains(mousePosF.x, mousePosF.y)) {
                            maze.dialogueBoxState = 4;
                        }
                        else if (giveUpBounds.contains(mousePosF.x, mousePosF.y)) {
                            maze.grid[doorRow][doorCol].first = WALL;
                            maze.isDialogueBoxDisplayed = false;
                            maze.dialogueBoxState = 1;
                        }
                    }
                    else if (maze.dialogueBoxState == 4) {
                        clickInsideButton = false;

                        sf::FloatRect liarBounds((viewSize.x + 100) / 2.0f, dialogueBoxPosition.y + 270, 50, 30);
                        sf::FloatRect honestBounds((viewSize.x - 250) / 2.0f, dialogueBoxPosition.y + 270, 80, 30);

                        if (liarBounds.contains(mousePosF.x, mousePosF.y)) {
                            playerGuess = true;
                            clickInsideButton = true;
                        }
                        else if (honestBounds.contains(mousePosF.x, mousePosF.y)) {
                            playerGuess = false;
                            clickInsideButton = true;
                        }

                        if (clickInsideButton == true) {
                            if (playerGuess == maze.grid[doorRow][doorCol].second.second.second.first) {
                                sf::Text correctText;
                                correctText.setFont(font);
                                correctText.setCharacterSize(24);
                                correctText.setFillColor(sf::Color::Green);
                                correctText.setString("Correct!");
                                correctText.setPosition(dialogueBoxPosition.x + 825, dialogueBoxPosition.y + 275);
                                window.draw(correctText);
                                window.display();
                                sf::sleep(sf::seconds(1));
                                doorOpenSound.play();
                                maze.grid[doorRow][doorCol].first = EMPTY;
                            }
                            else {
                                sf::Text incorrectText;
                                incorrectText.setFont(font);
                                incorrectText.setCharacterSize(24);
                                incorrectText.setFillColor(sf::Color::Red);
                                incorrectText.setString("Incorrect! :(");
                                incorrectText.setPosition(dialogueBoxPosition.x + 825, dialogueBoxPosition.y + 275);
                                window.draw(incorrectText);
                                window.display();
                                sf::sleep(sf::seconds(1));
                                doorFailedSound.play();
                                maze.grid[doorRow][doorCol].first = WALL;
                            }
                            maze.isDialogueBoxDisplayed = false;
                            maze.dialogueBoxState = 1;
                        }
                    }
                    else if (maze.dialogueBoxState == 5) {
                        sf::FloatRect playAgainBounds((viewSize.x - 250) / 2.0f, dialogueBoxPosition.y + 270, 100, 30);
                        sf::FloatRect exitBounds((viewSize.x + 100) / 2.0f, dialogueBoxPosition.y + 270, 50, 30);

                        if (playAgainBounds.contains(mousePosF.x, mousePosF.y)) {
                            // Restart the game
                            srand(static_cast<unsigned int>(std::time(0)));
                            maze = Maze();

                            // Search for the first .gguf file in the "model" directory
                            std::string modelPath;
                            for (const auto& entry : std::filesystem::directory_iterator("./model")) {
                                if (entry.path().extension() == ".gguf") {
                                    modelPath = entry.path().string();
                                    break;
                                }
                            }

                            if (modelPath.empty()) {
                                std::cout << "Error: no .gguf model file found in the 'model' directory" << std::endl;
                                return 1;
                            }

                            // Load the model
                            maze.model = llama_load_model_from_file(modelPath.c_str(), model_params);
                            if (maze.model == NULL) {
                                std::cout << "Error: unable to load model" << std::endl;
                                return 1;
                            }

                            // Reset the context
                            maze.ctx_params.seed = 1234;
                            maze.ctx_params.n_ctx = 2048;
                            maze.ctx_params.n_threads = maze.params.n_threads;
                            maze.ctx_params.n_threads_batch = maze.params.n_threads_batch == -1 ? maze.params.n_threads : maze.params.n_threads_batch;
                            maze.ctx = llama_new_context_with_model(maze.model, maze.ctx_params);
                            maze.check_ctx = llama_new_context_with_model(maze.model, maze.ctx_params);

                            // Stop the win sound
                            winSound.stop();

                            if (maze.ctx == NULL) {
                                std::cout << "Error: failed to create the llama_context" << std::endl;
                                return 1;
                            }
                            if (maze.check_ctx == NULL) {
                                std::cout << "Error: failed to create the llama_context" << std::endl;
                                return 1;
                            }
                        }
                        else if (exitBounds.contains(mousePosF.x, mousePosF.y)) {
                            // Close the window
                            window.clear();
                            window.close();
                        }
                    }
                    else if (maze.dialogueBoxState == 6) {
                        sf::FloatRect playAgainBounds((viewSize.x - 250) / 2.0f, dialogueBoxPosition.y + 270, 100, 30);
                        sf::FloatRect exitBounds((viewSize.x + 100) / 2.0f, dialogueBoxPosition.y + 270, 50, 30);

                        if (playAgainBounds.contains(mousePosF.x, mousePosF.y)) {
                            // Restart the game
                            srand(static_cast<unsigned int>(std::time(0)));
                            maze = Maze();

                            // Search for the first .gguf file in the "model" directory
                            std::string modelPath;
                            for (const auto& entry : std::filesystem::directory_iterator("./model")) {
                                if (entry.path().extension() == ".gguf") {
                                    modelPath = entry.path().string();
                                    break;
                                }
                            }

                            if (modelPath.empty()) {
                                std::cout << "Error: no .gguf model file found in the 'model' directory" << std::endl;
                                return 1;
                            }

                            // Load the model
                            maze.model = llama_load_model_from_file(modelPath.c_str(), model_params);
                            if (maze.model == NULL) {
                                std::cout << "Error: unable to load model" << std::endl;
                                return 1;
                            }

                            // Reset the context
                            maze.ctx_params.seed = 1234;
                            maze.ctx_params.n_ctx = 2048;
                            maze.ctx_params.n_threads = maze.params.n_threads;
                            maze.ctx_params.n_threads_batch = maze.params.n_threads_batch == -1 ? maze.params.n_threads : maze.params.n_threads_batch;
                            maze.ctx = llama_new_context_with_model(maze.model, maze.ctx_params);
                            maze.check_ctx = llama_new_context_with_model(maze.model, maze.ctx_params);

                            // Stop the lose sound
                            loseSound.stop();

                            if (maze.ctx == NULL) {
                                std::cout << "Error: failed to create the llama_context" << std::endl;
                                return 1;
                            }
                            if (maze.check_ctx == NULL) {
                                std::cout << "Error: failed to create the llama_context" << std::endl;
                                return 1;
                            }
                        }
                        else if (exitBounds.contains(mousePosF.x, mousePosF.y)) {
                            // Close the window
                            window.clear();
                            window.close();
                        }
                    }
                }
            }
            else if (event.type == sf::Event::TextEntered) {
                if (maze.getDialogueBoxDisplayed() && maze.dialogueBoxState == 2) {
                    if (event.text.unicode == 8) { // Check for backspace key
                        if (!maze.playerInput.isEmpty()) {
                            maze.playerInput.erase(maze.playerInput.getSize() - 1, 1);
                        }
                    }
                    else if (event.text.unicode < 128) {
                        maze.playerInput += static_cast<char>(event.text.unicode);
                    }
                }
            }
            else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::F11) {
                    if (window.getSize() == sf::Vector2u(15 * TILE_SIZE, 15 * TILE_SIZE)) {
                        window.create(sf::VideoMode::getFullscreenModes()[0], "Lucy's Labyrinth", sf::Style::Fullscreen);
                    }
                    else {
                        window.create(sf::VideoMode(15 * TILE_SIZE, 15 * TILE_SIZE), "Lucy's Labyrinth");
                    }
                    window.setVerticalSyncEnabled(true);
                }
                if (maze.getDialogueBoxDisplayed() && maze.dialogueBoxState == 2) {
                    if (event.key.code == sf::Keyboard::Enter) {
                        maze.isDoorThinking = true; // Set isDoorThinking to true
                        maze.display(window);
                        window.display();

                        maze.response = maze.generateResponse(maze.grid[doorRow][doorCol].second.second.first, maze.grid[doorRow][doorCol].second.second.second.first);

                        maze.isDoorThinking = false; // Set isDoorThinking back to false
                        maze.dialogueBoxState = 3;
                        maze.playerInput.clear();
                    }
                }
                else {
                    if (!maze.getDialogueBoxDisplayed()) {
                        if (event.key.code == sf::Keyboard::W)
                            maze.movePlayer('w', window);
                        else if (event.key.code == sf::Keyboard::S)
                            maze.movePlayer('s', window);
                        else if (event.key.code == sf::Keyboard::A)
                            maze.movePlayer('a', window);
                        else if (event.key.code == sf::Keyboard::D)
                            maze.movePlayer('d', window);
                    }
                }
            }
        }

        window.clear();
        if (maze.startupScreenDisplayed) {
            sf::Sprite startupScreenSprite(maze.startupScreenTexture);

            // Center the startup screen sprite
            sf::Vector2u windowSize = window.getSize();
            sf::Vector2u spriteSize = startupScreenSprite.getTexture()->getSize();
            startupScreenSprite.setPosition(
                (windowSize.x - spriteSize.x) / 2.0f,
                (windowSize.y - spriteSize.y) / 2.0f
            );

            window.draw(startupScreenSprite);
        }
        else {
            maze.display(window);
        }
        window.display();
    }
    return 0;
}
