
#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <set>
#include <condition_variable>
#include <curl/curl.h>
#include <string>

using namespace std;

// Define a structure to hold the information about a crawled page
struct CrawledPage {
    string url;
    string content;
};

// Queue to hold URLs to be crawled
queue<string> url_queue;

// Set to store visited URLs to avoid revisiting
set<string> visited_urls;

// Mutex to protect shared data structures
mutex url_queue_mutex;
mutex visited_urls_mutex;

// Condition variable to signal when the queue is not empty
condition_variable cv;

// Function to download the content of a URL
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    string *content = static_cast<string*>(userp);
    size_t realsize = size * nmemb;
    content->append(static_cast<char*>(contents), realsize);
    return realsize;
}

// Worker thread function that crawls URLs from the queue
void crawler_thread() {
    while (true) {
        unique_lock<mutex> lock(url_queue_mutex);
        cv.wait(lock, []() { return !url_queue.empty(); });

        string url = url_queue.front();
        url_queue.pop();

        lock.unlock();

        // Check if the URL has already been visited
        {
            unique_lock<mutex> lock(visited_urls_mutex);
            if (visited_urls.find(url) != visited_urls.end()) {
                continue;
            }
            visited_urls.insert(url);
        }

        cout << "Crawling: " << url << endl;

        CURL *curl;
        CURLcode res;
        string content;

        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);

            // Perform the HTTP request
            res = curl_easy_perform(curl);

            // Check for errors
            if (res != CURLE_OK) {
                cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
            }

            // Cleanup
            curl_easy_cleanup(curl);
        }

        // Process the crawled page content (e.g., extract links and add them to the queue)
        // For simplicity, we will just print the content in this example.
        cout << "Content:" << endl;
        cout << content << endl;
    }
}

int main() {
    const int num_threads = 10; // Number of crawler threads

    // Initialize CURL
    curl_global_init(CURL_GLOBAL_ALL);

    // Start crawler threads
    vector<thread> threads;
    for (int i = 0; i < num_threads; i++) {
        threads.push_back(thread(crawler_thread));
    }

    // Seed the queue with initial URLs to crawl
    url_queue.push("https://www.example.com");
    cv.notify_all();

    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }

    // Cleanup
    curl_global_cleanup();

    return 0;
}
                  