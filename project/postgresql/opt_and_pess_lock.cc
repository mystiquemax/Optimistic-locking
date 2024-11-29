#include <pqxx/connection.hxx>
#include <pqxx/pqxx>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <random>

struct Account {
    int id;
    std::string username;
    double balance;
    int version;
};

struct DepositRequest {
    std::string username;
    double amount;
};

int getRandomNumber(int min, int max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}
// Pessimistic Locking Implementation
class AccountRepositoryPess {
public:
    AccountRepositoryPess(pqxx::connection& conn) : conn(conn) {}

    void deposit(const DepositRequest& request) {
        pqxx::work txn(conn);
        try {
            // Select account with pessimistic lock
            pqxx::result res = txn.exec_prepared("select_account_lock", request.username);
            if (res.empty()) {
                throw std::runtime_error("Account not found");
            }

            Account account;
            for (const auto& row : res) {
                account.id = row["id"].as<int>();
                account.username = row["username"].as<std::string>();
                account.balance = row["balance"].as<double>();
                account.version = row["version"].as<int>();
            }

            // Modify balance
            account.balance += request.amount;

            // Update account
            txn.exec_prepared("deposit_account", account.balance, account.id);
            txn.commit();
        } catch (const std::exception& e) {
            txn.abort();
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

private:
    pqxx::connection& conn;
};

// Optimistic Locking Implementation
class AccountRepositoryOpt {
public:
    AccountRepositoryOpt(pqxx::connection& conn) : conn(conn) {}

    void depositOpt(const DepositRequest& request) {
       while(true){
        pqxx::work txn(conn);
        try {
            // Select account
            pqxx::result res = txn.exec_prepared("select_account_by_username", request.username);
            if (res.empty()) {
                throw std::runtime_error("Account not found");
            }

            Account account;
            for (const auto& row : res) {
                account.id = row["id"].as<int>();
                account.username = row["username"].as<std::string>();
                account.balance = row["balance"].as<double>();
                account.version = row["version"].as<int>();
            }

            // Modify balance
            account.balance += request.amount;

            std::this_thread::sleep_for(std::chrono::milliseconds(getRandomNumber(10, 20)));

            // Update account with optimistic lock
            pqxx::result update_res = txn.exec_prepared("deposit_account_opt", account.balance, account.id, account.version);
            if (update_res.affected_rows() == 0) {
                // Retry if no rows were affected
                txn.abort();  
            } else {
                txn.commit();
                return;
            }
        } catch (const std::exception& e) {
            txn.abort();
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
    }
private:
    pqxx::connection& conn;
};

void testDataPess(int user_count) {
    std::mutex mtx;
    std::vector<std::thread> threads;
    for (int i = 0; i < user_count; i++) {
        threads.emplace_back([ i, &mtx, user_count] {
            try {
                // Create a new connection for each thread
                pqxx::connection conn;
                conn.prepare("select_account_lock", "SELECT * FROM account WHERE username = $1 LIMIT 1 FOR NO KEY UPDATE");
                conn.prepare("deposit_account", "UPDATE account SET balance = $1, version = version+1 WHERE id = $2");
                AccountRepositoryPess localRepo(conn);
                DepositRequest req{std::to_string( i + 1), 1000};
                localRepo.deposit(req);
                conn.disconnect();
            } catch (const std::exception& e) {
                std::lock_guard<std::mutex> lock(mtx);
                std::cerr << "Error: " << e.what() << std::endl;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

void testDataOpt( int user_count) {
    std::mutex mtx;
    std::vector<std::thread> threads;
    for (int i = 0; i < user_count; i++) {
        threads.emplace_back([i, &mtx, user_count] {
            try {
                // Create a new connection for each thread
                pqxx::connection conn;
                conn.prepare("select_account_by_username", "SELECT * FROM account WHERE username = $1 LIMIT 1");
                conn.prepare("deposit_account_opt", "UPDATE account SET balance = $1, version = version+1 WHERE id = $2 AND version = $3");
                AccountRepositoryOpt localRepo(conn);
                DepositRequest req{std::to_string(i+1), 1000};
                localRepo.depositOpt(req);
                conn.disconnect();
            } catch (const std::exception& e) {
                std::lock_guard<std::mutex> lock(mtx);
                std::cerr << "Error: " << e.what() << std::endl;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

void init_table(){
    try {
        pqxx::connection conn;
        if (conn.is_open()) {
            std::cout << "Opened database successfully: " << conn.dbname() << std::endl;
            pqxx::work txn(conn);

            txn.exec(R"(
            CREATE TABLE IF NOT EXIST account (
                id SERIAL PRIMARY KEY,
                username VARCHAR(100) UNIQUE NOT NULL,
                balance NUMERIC(10, 2) DEFAULT 0.0,
                version INT DEFAULT 0
            ))");
            txn.commit();
        } else {
            std::cout << "Failed to open database" << std::endl;
        }

        conn.disconnect();
        std::cout << "Disconnected from database" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

void insertData(int num_elements){
    try{
    pqxx::connection conn; // connects to the default database
    if(conn.is_open()){
        pqxx::work txn(conn);
        conn.prepare("insert_account", "INSERT INTO account (username, balance, version) VALUES ($1, $2, $3)");
        for (int i = 0; i < num_elements; i++) {
            std::string username = std::to_string((i%num_elements) + 1);
            double balance = 1000.00;
            int version = 0;
            txn.exec_prepared("insert_account", username, balance, version);
        }  
        txn.commit();
        std::cout << "Successfully inserted 1000 items into the account table." << std::endl;
    } else{
        std::cout << "Failed to open database" << std::endl;
    }
    conn.disconnect();
    } catch(const std::exception &e){
        std::cerr << e.what() << std::endl;
    }
}

void deleteData(){
      try {
        pqxx::connection conn;
        if (conn.is_open()) {
            pqxx::work txn(conn);
            txn.exec("DELETE FROM account");
            txn.commit();
            std::cout << "Deleted all rows from the table successfully." << std::endl;
        } else {
            std::cout << "Failed to open database" << std::endl;
        }

        conn.disconnect();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

// Main function to run tests
int main() {
    //init_table();
    //insertData(1000);
    std::cout << "Testing pessimistic locking:" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    testDataPess(100);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_ns = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Execution time: " << duration_ns << " ms" << std::endl;

    std::cout << "Testing optimistic locking:" << std::endl;
    start = std::chrono::high_resolution_clock::now();
    testDataOpt(100);
    end = std::chrono::high_resolution_clock::now();
    duration_ns = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Execution time: " << duration_ns << " ms" << std::endl;
    //deleteData(); 
    return 0;
}
