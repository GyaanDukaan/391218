#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <set>
#include <thread>
#include <unordered_map>
#include <curl/curl.h>

#define MAX_THREADS 10

// Define a structure to hold URL details
struct URLData {
    std::string url;
    std::string content;
};

// Function to fetch content from a URL
std::string fetchURL(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string content;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void* contents, size_t size, size_t nmemb, void* userp) {
            content += static_cast<char*>(contents);
            return size * nmemb;
        });
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
    } else {
        std::cerr << "curl_easy_init() failed" << std::endl;
    }

    return content;
}

// Function to parse HTML and extract links
std::set<std::string> parseLinks(const std::string& content) {
    std::set<std::string> links;

    // Simple link extraction logic for demonstration purposes
    size_t pos = 0;
    while ((pos = content.find("href=\"", pos)) != std::string::npos) {
        pos += 6; // Skip "href="
        size_t end = content.find("\"", pos);
        std::string link = content.substr(pos, end - pos);

        // Handle relative URLs
        if (link.find("http") == std::string::npos) {
            link = "http://example.com/" + link;
        }

        links.insert(link);
        pos = end;
    }

    return links;
}

// Crawler thread function
void crawlThread(std::queue<std::string>& urlQueue, std::mutex& queueMutex, std::condition_variable& cv, std::set<std::string>& visited) {
    while (true) {
        std::string url;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [&] { return !urlQueue.empty() || !visited.empty(); });

            if (urlQueue.empty() && visited.empty()) {
                return; // Exit if the queue is empty and no more URLs to visit
            }

            url = urlQueue.front();
            urlQueue.pop();
        }

        if (visited.find(url) != visited.end()) {
            continue; // Skip already visited URLs
        }

        std::cout << "Crawling: " << url << std::endl;

        std::string content = fetchURL(url);
        if (!content.empty()) {
            std::set<std::string> links = parseLinks(content);

            {
                std::unique_lock<std::mutex> lock(queueMutex);
                for (const auto& link : links) {
                    if (visited.find(link) == visited.end()) {
                        urlQueue.push(link);
                    }
                }
                visited.insert(url);
            }
        }
    }
}

int main() {
    std::queue<std::string> urlQueue;
    std::mutex queueMutex;
    std::condition_variable cv;
    std::set<std::string> visited;

    // Initialize the queue with a starting URL
    urlQueue.push("http://example.com");

    // Create a pool of worker threads
    std::vector<std::thread> threads(MAX_THREADS);

    for (auto& thread : threads) {
        thread = std::thread(crawlThread, std::ref(urlQueue), std::ref(queueMutex), std::ref(cv), std::ref(visited));
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    std::cout << "Crawling completed." << std::endl;

    return 0;
}