#include "database.hpp"

void Database::populateFromFile(std::ifstream& file) {
    std::cout << "Populating database from file ..." << std::endl;

    // read in lines in pairs. The first line is the name, the next is the phone number
    std::string line;
    while (std::getline(file, line)) {
        std::u32string name = normalizeToUTF32(line);

        std::getline(file, line);

        if(validateName(name) && validatePhoneNumber(line)) {
            // if the name and phone number are validated this constructs a new user in the hash map
            // if the user already exists skip inserting
            User user(name, line);

            if(std::find(Users.begin(), Users.end(), user) != Users.end()) {
                std::cout << "User " << u32ToString(name) << " already exists" << std::endl;
                continue;
            }
            
            Users.emplace_back(std::move(user));
        }
    }

    std::cout << std::endl;
}

bool Database::getCommand() {
    std::string input;

    std::cout << "Please enter a command:" << std::endl
                << "ADD" << std::endl
                << "DEL" << std::endl
                << "LIST" << std::endl
                << "EXIT" << std::endl << std::endl;

    // get the input and convert it to all uppercase
    std::getline(std::cin, input);
    std::transform(input.begin(), input.end(), input.begin(), ::toupper);

    std::cout << std::endl;

    if (input.substr(0, 3) == "ADD") {
        add();
    } else if (input.substr(0, 3) == "DEL") {
        del();
    } else if (input.substr(0, 4) == "LIST") {
        list();
    } else if (input.substr(0, 4) == "EXIT") {
        return false;
    } else {
        std::cout << "Invalid input" << std::endl;
    }

    std::cout << std::endl;

    return true;
}

std::u32string Database::normalizeToUTF32(const std::string& str) const {
     // first the name is converted from utf-8 to utf-32
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf32conv;
    auto utf32 = utf32conv.from_bytes(str);

    // next the name is normalized. This will solve some problems of equivialency when performing operations of the set
    // more can be seen about this here http://unicode.org/reports/tr15/#Norm_Forms
    // This performs type c normalization
    // I have just included the two required files from https://github.com/ufal/unilib to make this work
    ufal::unilib::uninorms::nfc(utf32);

    // Next I replace all space characters with the normal space character
    const static std::vector<uint32_t> spaceChars = {9, 160, 5760, 6158, 8192, 8193, 8194, 8195, 8196, 8197, 8198, 8199, 8200, 8201, 8202, 8203, 8239, 8287, 12288, 65279};
    for (auto& c : utf32) {
        if (std::binary_search(spaceChars.begin(), spaceChars.end(), c)) {
            // sets it to the value of the normal space character
            c = 32;
        }
    }

    // Replaces concecutive spaces with a single space
    std::u32string::iterator new_end = std::unique(utf32.begin(), utf32.end(), [](const char32_t& lhs, const char32_t& rhs) { return (lhs == rhs) && (lhs == 32); });
    utf32.erase(new_end, utf32.end());

    // If the first character is a space, it is removed
    if (utf32[0] == 32) utf32.erase(utf32.begin());

    // If the last character is a space, it is removed
    // Checks if the string is empty first because a single whoule be removed in the previous step
    if (utf32.size() && utf32[utf32.size() - 1] == 32) utf32.erase(utf32.end() - 1);

    // At this point if a name was all whitespace, it would be now be empty

    return utf32;
}

bool Database::validateName(std::u32string& name) const {
    // after doing much research i have decided to only ban a handful of characters from names
    // most of the banned chars are Unicode control characters and could never be in a name
    // The rest are characters that do not display and do effect the appearance of the name ie. the right-to-left mark
    // I realize this list allows through many problamatic or malicious string but they are not dangerous in the context of this program
    // This decision was initialy based on not wanting to exclude any real names or
    // potential real names (hence accepting most UTF-8 supported chars)
    // This was then backed up by finding out that some places (ie. Kentucky) have potentialy no restrictions on baby names
    // Kentucky naming law - https://apps.legislature.ky.gov/law/statutes/statute.aspx?id=50029
    
    if(name.empty()) return false;

    for (const auto& c : name) {
        if (c < 32) return false;
        if (c > 126 && c < 161) return false;
        if (c > 8286 && c < 8298) return false;
        if (c > 8432 && c < 8448) return false;
        if (c > 917503 && c < 921601) return false;
    }

    return true;
}

bool Database::validatePhoneNumber(std::string& phoneNumber) const {
    std::string extention;
    std::smatch matches;

    // If there anything other than letters, numbers, or a handfull of punctuation and spaces return false
    // This is a sanity check to throw out things that are obviously not numbers
    if(std::regex_search(phoneNumber, std::regex("[^A-Za-z0-9\\+\\(\\)\\-., ]"))) return false;

    // If there is letters it could have an extention
    // If there is, then parse it sepratly
    if(std::regex_search(phoneNumber, std::regex("[A-Za-z,]"))) {
        // Make all letters lowercase for ease of parsing
        std::transform(phoneNumber.begin(), phoneNumber.end(), phoneNumber.begin(), ::tolower);

        // If there is no "x" there will be no extention
        // Every way I could find to show an extention had an "x" and only one
        // If there are no x's or more than one return false
        std::regex_search(phoneNumber, matches, std::regex("x"));
        if(matches.size() != 1) return false;

        // Search first for extentions that say "extention" and replace it with "ext."
        // The "." at the end of "ext" makes sure that "extention." will fail later because of the extra "."
        phoneNumber = std::regex_replace(phoneNumber, std::regex("extention"), "ext.", std::regex_constants::format_first_only);
        // Search for extentions specified by "ext" or "ext." and replace with "x"
        phoneNumber = std::regex_replace(phoneNumber, std::regex("ext\\.?"), "x", std::regex_constants::format_first_only);

        // If there are any reamining letters other than one x, return false
        if(std::regex_search(phoneNumber, std::regex("[a-wyz]+"))) return false;

        // Match from the x to the end of the string
        // This is the extention
        std::regex_search(phoneNumber, matches, std::regex("x.*"));
        extention = matches[0];
        phoneNumber = phoneNumber.substr(0, phoneNumber.size() - extention.size());

        // An extention must now fit the format of "x" followed one or 0 spaces and between 1 and 15 digits
        // I could not find an offical limit of the length of an extention and even found some sources saying
        // they used some really long ones for niche uses like masking the number
        // I have chosen to only allow a max of 15 digits because it is long enough to use it as a mask for a max length phone number
        if(!std::regex_match(extention, std::regex("x\\s?[0-9]{1,15}"))) return false;

        extention = std::regex_replace(extention, std::regex("\\s"), "");
    }

    // Remove trailing spaces
    if(phoneNumber.back() == ' ') phoneNumber.pop_back();

    // Sometimes the country code is separated from the number by a "."
    // If there is more than one "." in the string, return false
    std::regex_search(phoneNumber, matches, std::regex("\\."));
    if(matches.size() > 1) return false;

    // If there is "(" or ")" but not in the pattern of "([0-9]{3})" return false
    std::regex_search(phoneNumber, matches, std::regex("\\(|\\)"));
    if(matches.size() && !std::regex_search(phoneNumber, std::regex("\\([0-9]{3}\\)"))) return false;
    
    
    // replace everything except + and 0-9 with spaces
    phoneNumber = std::regex_replace(phoneNumber, std::regex("[^0-9\\+]+"), " ");

    // If there is a gap with two spaces, reduce it to one
    // This accounts for cases like (972)-964-4333
    phoneNumber = std::regex_replace(phoneNumber, std::regex("\\s\\s"), " ");

    // Remove possible leading spaces
    if(phoneNumber[0] == ' ') phoneNumber = phoneNumber.substr(1);

    // If it starts with a "+" parse it as an international number
    if(phoneNumber[0] == '+' && phoneNumber.substr(0, 3) != "+1 ") {
        // Remove all spaces
        phoneNumber = std::regex_replace(phoneNumber, std::regex("\\s"), "");

        // I decided to not create parsers based on spesfic country codes
        // The max length of an international number is 15 digits and the shortest is 7
        if(!std::regex_match(phoneNumber, std::regex("\\+[0-9]{7,15}"))) return false;

        // If it doesnt start with a vaild country code, return false
        static const std::regex countryCodeRegex("^\\+(001|297|93|244|1264|358|355|376|971|54|374|1684|1268|61|43|994|257|32|229|226|880|359|973|1242|387|590|375|501|1441|591|55|1246|673|975|267|236|1|61|41|56|86|225|237|243|242|682|57|269|238|506|53|5999|61|1345|357|420|49|253|1767|45|1809|1829|1849|213|593|20|291|212|34|372|251|358|679|500|33|298|691|241|44|995|44|233|350|224|590|220|245|240|30|1473|299|502|594|1671|592|852|504|385|509|36|62|44|91|246|353|98|964|354|972|39|1876|44|962|81|76|77|254|996|855|686|1869|82|383|965|856|961|231|218|1758|423|94|266|370|352|371|853|590|212|377|373|261|960|52|692|389|223|356|95|382|976|1670|258|222|1664|596|230|265|60|262|264|687|227|672|234|505|683|31|47|977|674|64|968|92|507|64|51|63|680|675|48|1787|1939|850|351|595|970|689|974|262|40|7|250|966|249|221|65|500|4779|677|232|503|378|252|508|381|211|239|597|421|386|46|268|1721|248|963|1649|235|228|66|992|690|993|670|676|1868|216|90|688|886|255|256|380|598|1|998|3906698|379|1784|58|1284|1340|84|678|681|685|967|27|260|263)");
        if(!std::regex_search(phoneNumber, countryCodeRegex)) return false;

        // If it doesnt have a North American country code return true
        // Otherwise remove it and continue
        if(!std::regex_search(phoneNumber, std::regex("^\\+001"))) {
            phoneNumber += extention;
            return true;
        }
        phoneNumber = phoneNumber.substr(4);

        // Reintoduce the spaces after the area code and before the phone number
        phoneNumber.insert(6, " ");
        phoneNumber.insert(3, " ");
    }

    if (phoneNumber.substr(0, 3) == "+1 ") {
        phoneNumber = phoneNumber.substr(3);
    }

    // Parse as an North American number
    // A genral check for a valid number
    if(!std::regex_match(phoneNumber, std::regex("[2-9][0-9]{2} [0-9]{3} [0-9]{4} ?"))) return false;

    // List of in-use us area codes from:
    // https://en.wikipedia.org/wiki/List_of_North_American_Numbering_Plan_area_codes
    static const std::regex areaCodeRegex("^(201|202|203|204|205|206|207|208|209|210|211|212|213|214|215|216|217|218|219|220|223|224|225|226|227|228|229|231|234|236|239|240|242|246|248|249|250|251|252|253|254|256|260|262|263|264|267|268|269|270|272|274|276|278|279|281|283|284|289|301|302|303|304|305|306|307|308|309|310|311|312|313|314|315|316|317|318|319|320|321|323|325|326|327|330|331|332|334|336|337|339|340|341|343|345|346|347|351|352|354|360|361|363|364|365|367|368|369|380|382|385|386|387|401|402|403|404|405|406|407|408|409|410|411|412|413|414|415|416|417|418|419|423|424|425|428|430|431|432|434|435|437|438|440|441|442|443|445|447|448|450|456|458|463|464|468|469|470|472|473|474|475|478|479|480|484|500|501|502|503|504|505|506|507|508|509|510|511|512|513|514|515|516|517|518|519|520|521|522|523|524|525|526|530|531|532|533|534|535|538|539|540|541|544|545|546|547|548|549|550|551|555|556|557|558|559|561|562|563|564|566|567|569|570|571|572|573|574|575|577|578|579|580|581|582|584|585|586|587|588|589|600|601|602|603|604|605|606|607|608|609|610|611|612|613|614|615|616|617|618|619|620|622|623|626|627|628|629|630|631|633|636|639|640|641|644|646|647|649|650|651|655|656|657|658|659|660|661|662|664|667|669|670|671|672|677|678|679|680|681|682|683|684|688|689|700|701|702|703|704|705|706|707|708|709|710|711|712|713|714|715|716|717|718|719|720|721|724|725|726|727|730|731|732|734|737|740|742|743|747|753|754|757|758|760|762|763|764|765|767|769|770|771|772|773|774|775|778|779|780|781|782|784|785|786|787|800|801|802|803|804|805|806|807|808|809|810|811|812|813|814|815|816|817|818|819|820|822|825|826|828|829|830|831|832|833|835|838|839|840|843|844|845|847|848|849|850|854|855|856|857|858|859|860|861|862|863|864|865|866|867|868|869|870|872|873|876|877|878|879|888|889|900|901|902|903|904|905|906|907|908|909|910|911|912|913|914|915|916|917|918|919|920|925|927|928|929|930|931|932|934|935|936|937|938|939|940|941|943|945|947|948|949|950|951|952|954|956|959|970|971|972|973|975|978|979|980|983|984|985|986|988|989)");
    if(!std::regex_search(phoneNumber, areaCodeRegex)) return false;
    
    phoneNumber += extention;

    return true;
}

std::string Database::u32ToString(const std::u32string &str) const {
    // return the std::string version of a std::u32string/
    // used to be able to print a stred name back to the console
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert;
    auto p = reinterpret_cast<const char32_t *>(str.data());
    return convert.to_bytes(p, p + str.size());
}

void Database::add() {
    // add a new entry to the database
    std::string nameInput;
    std::u32string name;
    std::string phoneNumber;

    // gets the name and standardizes it to a normalized UTF-32
    std::cout << "Please enter a name:" << std::endl;
    std::getline(std::cin, nameInput);
    name = normalizeToUTF32(nameInput);

    if (!validateName(name)) {
        std::cout << "The name you entered was invalid" << std::endl;
        return;
    }

    std::cout << "Please enter a phoneNumber:" << std::endl;
    std::getline(std::cin, phoneNumber);

    if (!validatePhoneNumber(phoneNumber)) {
        std::cout << "The phoneNumber you entered was invalid" << std::endl;
        return;
    }

    User user(name, phoneNumber);

    if(std::find(Users.begin(), Users.end(), user) != Users.end()) {
        std::cout << "User " << u32ToString(name) << " with that phone number already exists" << std::endl;
        return;
    }
    
    Users.emplace_back(std::move(user));
}

void Database::del() {
    // I know this fucntion has a lot of duplicate code but it would have probably been a mess of lambdas to do it different
    std::string input;
    std::cout << "Would you like to delete by (1) name or (2) phone number?" << std::endl;

    std::getline(std::cin, input);

    if (input.substr(0, 1) == "1") {
        std::string nameInput;
        std::u32string name;

        std::cout << "Please enter a name:" << std::endl;
        std::getline(std::cin, nameInput);
        name = normalizeToUTF32(nameInput);

        if (!validateName(name)) {
            std::cout << "The name you entered was invalid" << std::endl;
            return;
        }

        std::vector<std::list<User>::const_iterator> matches;
        auto i = Users.begin();
        while (i != Users.end()) {
            i = std::find_if(i, Users.end(), [name](const User& user) { return user.name == name; });
            if (i != Users.end()) matches.push_back(i++);
        }

        if (matches.empty()) {
            std::cout << "No users with that name were found" << std::endl;
            return;
        } else if (matches.size() == 1) {
            Users.erase(matches[0]);
        } else {
            std::cout << "Multiple users with that name were found, which one would you like to delete?" << std::endl;
            // print the names of the users and ask the user to select one
            for (int x = 0; x < matches.size(); x++) {
                std::cout << "(" << x + 1 << ") " << u32ToString(matches[x]->name) << " " << matches[x]->phoneNumber << std::endl;
            }
            std::getline(std::cin, input);
            int selection = std::strtol(input.c_str(), nullptr, 10);
            if (selection > matches.size() || selection < 1) {
                std::cout << "Invalid selection" << std::endl;
                return;
            }
            Users.erase(matches[selection - 1]);
        }

    } else if (input.substr(0, 1) == "2") {
        std::string phoneNumber;
        std::cout << "Please enter a phoneNumber:" << std::endl;
        std::getline(std::cin, phoneNumber);

        if (!validatePhoneNumber(phoneNumber)) {
            std::cout << "The phoneNumber you entered was invalid" << std::endl;
            return;
        }

        std::vector<std::list<User>::const_iterator> matches;
        auto i = Users.begin();
        while (i != Users.end()) {
            i = std::find_if(i, Users.end(), [phoneNumber](const User& user) { return user.phoneNumber == phoneNumber; });
            if (i != Users.end()) matches.push_back(i++);
        }

        if (matches.empty()) {
            std::cout << "No users with that phone number were found" << std::endl;
            return;
        } else if (matches.size() == 1) {
            Users.erase(matches[0]);
        } else {
            std::cout << "Multiple users with that phone number were found, which one would you like to delete?" << std::endl;
            // print the names of the users and ask the user to select one
            for (int x = 0; x < matches.size(); x++) {
                std::cout << "(" << x + 1 << ") " << u32ToString(matches[x]->name) << " " << matches[x]->phoneNumber << std::endl;
            }
            std::getline(std::cin, input);
            int selection = std::strtol(input.c_str(), nullptr, 10);
            if (selection > matches.size() || selection < 1) {
                std::cout << "Invalid selection" << std::endl;
                return;
            }
            Users.erase(matches[selection - 1]);
        }
    } else {
        std::cout << "Invalid input" << std::endl;
    }
}

void Database::list() {
    // loops through the unordered_set and prints each entry
    // supposedly O(n) and still somewhat efficent 
    for (const auto& user : Users) {
        std::cout << "Name: " << u32ToString(user.name) << std::endl;
        std::cout << "Phone Number: " << user.phoneNumber << std::endl;
    }
}