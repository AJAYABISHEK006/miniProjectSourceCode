
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

#define MAX_RECORDS       100
#define MIN_BALANCE       500.0
#define DAILY_LIMIT       5000.0
#define MAX_PIN_ATTEMPTS  3
#define OTP_LENGTH        6
#define MAX_BENEFICIARIES 5
#define FRAUD_THRESHOLD   10000.0
#define FRAUD_TXN_LIMIT   5          
#define SESSION_TIMEOUT   300        
#define SAVINGS_INTEREST  0.04       
#define CURRENT_INTEREST  0.00       

#define DATA_FILE         "credit.dat"
#define BACKUP_FILE       "backup.dat"
#define TEXT_FILE         "accounts.txt"
#define CSV_FILE          "accounts.csv"
#define TXN_FILE          "transactions.txt"
#define AUDIT_FILE        "audit.log"

typedef enum { ROLE_NONE=0, ROLE_USER=1, ROLE_ADMIN=2 } UserRole;
typedef enum { ACC_SAVINGS=1, ACC_CURRENT=2 } AccountType;
typedef enum { STATUS_ACTIVE=1, STATUS_FROZEN=2, STATUS_BLOCKED=3 } AccountStatus;

struct clientData {
    unsigned int  acctNum;
    char          lastName[15];
    char          firstName[10];
    double        balance;
    char          pin[7];              
    int           pinAttempts;         
    AccountType   accType;             
    AccountStatus status;              
    double        dailyWithdrawn;      
    char          lastTxnDate[11];     
    double        totalSpent;          
    int           txnCount;            
    int           recentTxnCount;      
    time_t        lastTxnTime;       
    char          beneficiaries[MAX_BENEFICIARIES][12]; 
    int           benefCount;
    char          profileImg[32];      
    double        monthlyDeposit;
    double        monthlyWithdraw;
    char          currentMonth[8];     
};

static struct {
    int          loggedIn;
    unsigned int acctNum;
    UserRole     role;
    time_t       loginTime;
    time_t       lastActivity;
} session = {0, 0, ROLE_NONE, 0, 0};

struct TxnRequest {
    char             type[20];
    unsigned int     acct;
    double           amount;
    unsigned int     toAcct;
    struct TxnRequest *next;
};
static struct TxnRequest *txnQueueHead = NULL;
static struct TxnRequest *txnQueueTail = NULL;

static const struct clientData BLANK_CLIENT = {
    0,"","",0.0,"000000",0,ACC_SAVINGS,STATUS_ACTIVE,0.0,
    "1970-01-01",0.0,0,0,0,{{""},{""},{""},{""},{""}},0,"none.jpg",0.0,0.0,"1970-01"
};

void textFile(FILE *readPtr);
void updateRecord(FILE *fPtr);
void newRecord(FILE *fPtr);
void deleteRecord(FILE *fPtr);

void listAllRecords(FILE *fPtr);
void searchByName(FILE *fPtr);
void accountSummary(FILE *fPtr);
void transferFunds(FILE *fPtr);
void applyInterest(FILE *fPtr);
void freezeAccount(FILE *fPtr);
void sortAccounts(FILE *fPtr);
void exportCSV(FILE *fPtr);
void importFromCSV(FILE *fPtr);
void dailyReport(FILE *fPtr);
void monthlySummary(FILE *fPtr);
void adminPanel(FILE *fPtr);
void manageBeneficiaries(FILE *fPtr);
void changePin(FILE *fPtr);
void checkDataIntegrity(FILE *fPtr);
void backupData(void);
void restoreData(FILE **fPtr);
void processQueue(FILE *fPtr);
void spendingAnalysis(FILE *fPtr);
void showSmartSuggestions(FILE *fPtr);

void        createDataFile(void);
int         readSlot(FILE *f, unsigned int a, struct clientData *out);
int         writeSlot(FILE *f, unsigned int a, const struct clientData *in);
int         pinAuth(FILE *fPtr, unsigned int acct);
int         generateOTP(void);
int         verifyOTP(int otp);
void        logTransaction(unsigned int acct, const char *type, double amt, double balAfter);
void        auditLog(const char *action);
int         fraudCheck(struct clientData *c, double amount);
void        getCurrentDate(char *buf);   
void        getCurrentMonth(char *buf);   
void        getCurrentDateTime(char *buf);
void        resetDailyIfNeeded(struct clientData *c);
int         sessionCheck(void);
void        sessionLogin(FILE *fPtr);
void        sessionLogout(void);
unsigned int enterChoice(UserRole role);

void ui_header(const char *title);
void ui_divider(void);
void ui_success(const char *msg);
void ui_error(const char *msg);
void ui_warn(const char *msg);
void ui_info(const char *msg);
void ui_tableHeader(void);
void ui_tableRow(const struct clientData *c);
void ui_tableFooter(void);
void ui_banner(void);
void clearInput(void);

void ui_divider(void) {
    printf("  %-62s\n","━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
}
void ui_thinline(void) {
    printf("  %-62s\n","──────────────────────────────────────────────────────────────");
}
void ui_header(const char *title) {
    printf("\n");
    ui_divider();
    printf("  %-60s\n", title);
    ui_thinline();
}
void ui_success(const char *msg) { printf("  [OK]  %s\n", msg); }
void ui_error(const char *msg)   { printf("  [ERR] %s\n", msg); }
void ui_warn(const char *msg)    { printf("  [!]   %s\n", msg); }
void ui_info(const char *msg)    { printf("  [i]   %s\n", msg); }

void ui_tableHeader(void) {
    printf("\n");
    printf("  %-5s %-15s %-10s %-10s %-10s %-8s %-7s\n",
           "Acct","Last Name","First","Balance","Type","Status","Frozen");
    ui_thinline();
}
void ui_tableRow(const struct clientData *c) {
    const char *type   = (c->accType==ACC_SAVINGS) ? "Savings" : "Current";
    const char *status = (c->status==STATUS_ACTIVE)  ? "Active"  :
                         (c->status==STATUS_FROZEN)  ? "FROZEN"  : "BLOCKED";
    printf("  %-5u %-15s %-10s %-10.2f %-10s %-8s\n",
           c->acctNum, c->lastName, c->firstName,
           c->balance, type, status);
}
void ui_tableFooter(void) { ui_thinline(); }

void ui_banner(void) {
    printf("\n\n");
    printf("  ╔══════════════════════════════════════════════════════════╗\n");
    printf("  ║                                                          ║\n");
    printf("  ║        NATIONAL BANK  —  ACCOUNT MANAGEMENT             ║\n");
    printf("  ║              Advanced Banking System v3.0               ║\n");
    printf("  ║                                                          ║\n");
    printf("  ╚══════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

void clearInput(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void getCurrentDate(char *buf) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buf, 11, "%Y-%m-%d", tm);
}
void getCurrentMonth(char *buf) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buf, 8, "%Y-%m", tm);
}
void getCurrentDateTime(char *buf) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buf, 25, "%Y-%m-%d %H:%M:%S", tm);
}

void createDataFile(void) {
    FILE *fp = fopen(DATA_FILE, "wb");
    if (!fp) { ui_error("Cannot create data file."); return; }
    for (int i = 0; i < MAX_RECORDS; i++)
        fwrite(&BLANK_CLIENT, sizeof(struct clientData), 1, fp);
    fclose(fp);
    /* seed admin account #1 */
    fp = fopen(DATA_FILE, "rb+");
    if (!fp) return;
    struct clientData admin = BLANK_CLIENT;
    admin.acctNum  = 1;
    strncpy(admin.lastName,  "ADMIN", 14);
    strncpy(admin.firstName, "Bank", 9);
    memcpy(admin.pin, "123456", 7);
    admin.balance  = 0.0;
    admin.accType  = ACC_CURRENT;
    admin.status   = STATUS_ACTIVE;
    fwrite(&admin, sizeof(struct clientData), 1, fp);
    fclose(fp);
}

int readSlot(FILE *f, unsigned int acct, struct clientData *out) {
    if (acct < 1 || acct > MAX_RECORDS) return 0;
    if (fseek(f, (long)(acct-1)*(long)sizeof(struct clientData), SEEK_SET) != 0) return 0;
    return (int)fread(out, sizeof(struct clientData), 1, f);
}

int writeSlot(FILE *f, unsigned int acct, const struct clientData *in) {
    if (acct < 1 || acct > MAX_RECORDS) return 0;
    if (fseek(f, (long)(acct-1)*(long)sizeof(struct clientData), SEEK_SET) != 0) return 0;
    return (int)fwrite(in, sizeof(struct clientData), 1, f);
}

void logTransaction(unsigned int acct, const char *type, double amt, double balAfter) {
    FILE *f = fopen(TXN_FILE, "a");
    if (!f) return;
    char dt[25]; getCurrentDateTime(dt);
    fprintf(f, "[%s] Acct#%-4u  %-12s  Amount:%10.2f  BalAfter:%10.2f\n",
            dt, acct, type, amt, balAfter);
    fclose(f);
}

void auditLog(const char *action) {
    FILE *f = fopen(AUDIT_FILE, "a");
    if (!f) return;
    char dt[25]; getCurrentDateTime(dt);
    unsigned int u = session.loggedIn ? session.acctNum : 0;
    fprintf(f, "[%s] User#%-4u  %s\n", dt, u, action);
    fclose(f);
}

int fraudCheck(struct clientData *c, double amount) {
    time_t now = time(NULL);
    /* too many transactions quickly */
    if (difftime(now, c->lastTxnTime) < 60) {
        c->recentTxnCount++;
        if (c->recentTxnCount > FRAUD_TXN_LIMIT) {
            ui_warn("FRAUD ALERT: Too many transactions in short time!");
            auditLog("FRAUD_FLAG: rapid transactions");
            c->status = STATUS_FROZEN;
            return 1;
        }
    } else {
        c->recentTxnCount = 1;
    }
    c->lastTxnTime = now;
    
    if (amount > FRAUD_THRESHOLD) {
        ui_warn("HIGH VALUE TRANSACTION ALERT: Amount exceeds Rs.10,000!");
        auditLog("HIGH_VALUE_TXN: large transfer flagged");
        printf("  Proceeding requires OTP verification.\n");
        return 2; 
    }
    return 0;
}

int pinAuth(FILE *fPtr, unsigned int acct) {
    struct clientData c;
    if (!readSlot(fPtr, acct, &c)) { ui_error("Cannot read account."); return 0; }
    if (c.status == STATUS_BLOCKED) {
        ui_error("Account is BLOCKED due to too many wrong PIN attempts.");
        return 0;
    }
    if (c.status == STATUS_FROZEN) {
        ui_error("Account is FROZEN. Contact admin.");
        return 0;
    }
    char entered[10];
    printf("  Enter PIN for account #%u: ", acct);
    scanf("%6s", entered);
    clearInput();
    if (strcmp(entered, c.pin) == 0) {
        c.pinAttempts = 0;
        writeSlot(fPtr, acct, &c);
        return 1;
    }
    c.pinAttempts++;
    if (c.pinAttempts >= MAX_PIN_ATTEMPTS) {
        c.status = STATUS_BLOCKED;
        ui_error("Account BLOCKED after 3 wrong PIN attempts!");
        auditLog("ACCOUNT_BLOCKED: wrong PIN x3");
    } else {
        char msg[60];
        snprintf(msg, sizeof(msg), "Wrong PIN. %d attempt(s) remaining.",
                 MAX_PIN_ATTEMPTS - c.pinAttempts);
        ui_error(msg);
    }
    writeSlot(fPtr, acct, &c);
    return 0;
}

int generateOTP(void) {
    srand((unsigned int)time(NULL));
    return 100000 + rand() % 900000;
}
int verifyOTP(int otp) {
    printf("  [OTP] Your OTP is: %d  (simulated SMS)\n", otp);
    printf("  Enter OTP to confirm: ");
    int entered; scanf("%d", &entered); clearInput();
    return (entered == otp);
}

int sessionCheck(void) {
    if (!session.loggedIn) return 0;
    time_t now = time(NULL);
    if (difftime(now, session.lastActivity) > SESSION_TIMEOUT) {
        ui_warn("Session timed out due to inactivity. Please log in again.");
        sessionLogout();
        return 0;
    }
    session.lastActivity = now;
    return 1;
}

void sessionLogin(FILE *fPtr) {
    unsigned int acct;
    ui_header("LOGIN");
    printf("  Account Number : "); scanf("%u", &acct); clearInput();
    if (!pinAuth(fPtr, acct)) return;
    struct clientData c;
    if (!readSlot(fPtr, acct, &c) || c.acctNum == 0) {
        ui_error("Account not found."); return;
    }
    session.loggedIn      = 1;
    session.acctNum       = acct;
    session.role          = (acct == 1) ? ROLE_ADMIN : ROLE_USER;
    session.loginTime     = time(NULL);
    session.lastActivity  = session.loginTime;
    char msg[60];
    snprintf(msg, sizeof(msg), "Welcome, %s %s!", c.firstName, c.lastName);
    ui_success(msg);
    auditLog("LOGIN");
}

void sessionLogout(void) {
    if (session.loggedIn) auditLog("LOGOUT");
    session.loggedIn = 0;
    session.acctNum  = 0;
    session.role     = ROLE_NONE;
}

void resetDailyIfNeeded(struct clientData *c) {
    char today[11]; getCurrentDate(today);
    if (strcmp(c->lastTxnDate, today) != 0) {
        c->dailyWithdrawn = 0.0;
        strncpy(c->lastTxnDate, today, 10);
    }
}

static void appendToTextFile(const struct clientData *c) {
    
    FILE *f = fopen(TEXT_FILE, "a");
    if (!f) return;
    
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz == 0)
        fprintf(f, "%-6s%-16s%-11s%12s %-8s %-8s\n",
                "Acct","Last Name","First Name","Balance","Type","Status");
    const char *type   = (c->accType==ACC_SAVINGS) ? "Savings" : "Current";
    const char *status = (c->status==STATUS_ACTIVE) ? "Active"  :
                         (c->status==STATUS_FROZEN) ? "FROZEN"  : "BLOCKED";
    fprintf(f, "%-6u%-16s%-11s%12.2f %-8s %-8s\n",
            c->acctNum, c->lastName, c->firstName, c->balance, type, status);
    fclose(f);
}

int main(void) {
    FILE        *cfPtr;
    unsigned int choice;

    ui_banner();

    cfPtr = fopen(DATA_FILE, "rb+");
    if (!cfPtr) {
        ui_info("Data file not found. Creating fresh database...");
        createDataFile();
        cfPtr = fopen(DATA_FILE, "rb+");
        if (!cfPtr) { ui_error("Fatal: cannot open data file."); return EXIT_FAILURE; }
        ui_success("Database initialised. Default admin: Acct#1, PIN: 123456");
    }

    backupData();

    while (!session.loggedIn) {
        printf("  Press L to login, Q to quit: ");
        char ch[4]; scanf("%3s", ch); clearInput();
        if (tolower(ch[0]) == 'q') { fclose(cfPtr); return 0; }
        if (tolower(ch[0]) == 'l') sessionLogin(cfPtr);
    }

    while (1) {
        if (!sessionCheck()) {
            
            while (!session.loggedIn) {
                printf("  Press L to login, Q to quit: ");
                char ch[4]; scanf("%3s", ch); clearInput();
                if (tolower(ch[0]) == 'q') { fclose(cfPtr); return 0; }
                if (tolower(ch[0]) == 'l') sessionLogin(cfPtr);
            }
        }

        choice = enterChoice(session.role);
        if (choice == 0) break; 

        switch (choice) {
            
            case 1:  textFile(cfPtr);       break;
            case 2:  updateRecord(cfPtr);   break;
            case 3:  newRecord(cfPtr);      break;
            case 4:  deleteRecord(cfPtr);   break;
            
            case 5:  listAllRecords(cfPtr); break;
            case 6:  searchByName(cfPtr);   break;
            case 7:  accountSummary(cfPtr); break;
            case 8:  transferFunds(cfPtr);  break;
    
            case 9:  applyInterest(cfPtr);  break;
            case 10: freezeAccount(cfPtr);  break;
            case 11: sortAccounts(cfPtr);   break;
            case 12: exportCSV(cfPtr);      break;
            case 13: importFromCSV(cfPtr);  break;
            case 14: dailyReport(cfPtr);    break;
            case 15: monthlySummary(cfPtr); break;
            case 16: manageBeneficiaries(cfPtr); break;
            case 17: changePin(cfPtr);      break;
            case 18: checkDataIntegrity(cfPtr); break;
            case 19: backupData();          break;
            case 20: restoreData(&cfPtr);   break;
            case 21: processQueue(cfPtr);   break;
            case 22: spendingAnalysis(cfPtr); break;
            case 23: showSmartSuggestions(cfPtr); break;
            case 24: if(session.role==ROLE_ADMIN){adminPanel(cfPtr);}else{ui_error("Admin only.");} break;
            case 25: sessionLogout();
                     ui_success("Logged out successfully.");
                     while (!session.loggedIn) {
                         printf("  L=login  Q=quit: ");
                         char ch[4]; scanf("%3s",ch); clearInput();
                         if(tolower(ch[0])=='q'){fclose(cfPtr);return 0;}
                         if(tolower(ch[0])=='l') sessionLogin(cfPtr);
                     }
                     break;
            case 99: 
                fclose(cfPtr);
                backupData();
                auditLog("PROGRAM_EXIT");
                ui_success("Goodbye! Data backed up.");
                return 0;
            default: ui_error("Invalid choice."); break;
        }
    }
    fclose(cfPtr);
    return 0;
}

unsigned int enterChoice(UserRole role) {
    ui_header("MAIN MENU");
    printf("  ── File / Export ──────────────────────────────────\n");
    printf("   1  Export accounts.txt (full regenerate)\n");
    printf("  12  Export accounts.csv\n");
    printf("  13  Import accounts from CSV\n");
    printf("  19  Backup database\n");
    printf("  20  Restore from backup\n");
    printf("  18  Data integrity check\n");
    printf("  ── Account Operations ─────────────────────────────\n");
    printf("   2  Update/Deposit/Withdraw\n");
    printf("   3  Add new account\n");
    printf("   4  Delete account\n");
    printf("  10  Freeze / Unfreeze account\n");
    printf("  17  Change PIN\n");
    printf("  16  Manage beneficiaries\n");
    printf("  ── View / Query ────────────────────────────────────\n");
    printf("   5  List all accounts\n");
    printf("   6  Search by name\n");
    printf("   7  Account statistics\n");
    printf("  11  Sort accounts (balance/name)\n");
    printf("  ── Transactions ────────────────────────────────────\n");
    printf("   8  Transfer funds (with OTP)\n");
    printf("   9  Apply interest to savings accounts\n");
    printf("  21  Process queued transactions\n");
    printf("  ── Analytics / Reports ─────────────────────────────\n");
    printf("  22  Spending pattern analysis\n");
    printf("  23  Smart suggestions\n");
    printf("  14  Daily report\n");
    printf("  15  Monthly summary\n");
    if (role == ROLE_ADMIN)
    printf("  24  Admin panel\n");
    printf("  ── Session ─────────────────────────────────────────\n");
    printf("  25  Switch user / Re-login\n");
    printf("  99  Exit\n");
    ui_thinline();
    printf("  Your choice: ");
    unsigned int c;
    if (scanf("%u", &c) != 1) { clearInput(); return 0; }
    clearInput();
    return c;
}

void textFile(FILE *readPtr) {
    FILE *writePtr = fopen(TEXT_FILE, "w");
    if (!writePtr) { ui_error("Cannot open accounts.txt."); return; }

    struct clientData client;
    fprintf(writePtr, "%-6s%-16s%-11s%12s %-8s %-8s\n",
            "Acct","Last Name","First Name","Balance","Type","Status");
    fprintf(writePtr, "%-62s\n",
        "--------------------------------------------------------------");

    rewind(readPtr);
    int count = 0;
    /* FIX BUG1: loop on fread return, not feof */
    while (fread(&client, sizeof(struct clientData), 1, readPtr) == 1) {
        if (client.acctNum != 0) {
            const char *type = (client.accType==ACC_SAVINGS)?"Savings":"Current";
            const char *st   = (client.status==STATUS_ACTIVE)?"Active":
                               (client.status==STATUS_FROZEN)?"FROZEN":"BLOCKED";
            fprintf(writePtr,"%-6u%-16s%-11s%12.2f %-8s %-8s\n",
                    client.acctNum, client.lastName, client.firstName,
                    client.balance, type, st);
            count++;
        }
    }
    fclose(writePtr);

    char msg[60]; snprintf(msg,sizeof(msg),"accounts.txt written — %d records.", count);
    ui_success(msg);
    auditLog("EXPORT_TXT");

    /* Task 3 verify: print file to screen */
    printf("\n  -- accounts.txt preview --\n");
    FILE *chk = fopen(TEXT_FILE,"r");
    if (chk) { char ln[120]; while(fgets(ln,sizeof(ln),chk)) printf("  %s",ln); fclose(chk); }
}

void updateRecord(FILE *fPtr) {
    if (!sessionCheck()) return;
    ui_header("UPDATE ACCOUNT BALANCE");

    unsigned int account;
    printf("  Account number (1-%d): ", MAX_RECORDS);
    
    if (scanf("%u", &account) != 1) { clearInput(); ui_error("Invalid input."); return; }
    clearInput();

    if (account < 1 || account > MAX_RECORDS) { ui_error("Account out of range."); return; }

    if (!pinAuth(fPtr, account)) return;

    struct clientData client;
    if (!readSlot(fPtr, account, &client) || client.acctNum == 0) {
        ui_error("Account has no information."); return;
    }

    ui_tableHeader(); ui_tableRow(&client); ui_tableFooter();

    printf("  1=Deposit  2=Withdraw  3=Online Transaction\n  Choice: ");
    int op; scanf("%d",&op); clearInput();

    double amount;
    printf("  Amount (Rs.): "); scanf("%lf",&amount); clearInput();
    if (amount <= 0) { ui_error("Amount must be positive."); return; }

    resetDailyIfNeeded(&client);

    if (op == 2 || op == 3) {
        
        if (op == 3) {
            int fc = fraudCheck(&client, amount);
            if (fc == 1) { writeSlot(fPtr,account,&client); return; } 
            if (fc == 2) {
                int otp = generateOTP();
                if (!verifyOTP(otp)) { ui_error("OTP failed. Transaction cancelled."); return; }
            }
        }
        if (client.balance - amount < MIN_BALANCE) {
            char msg[80];
            snprintf(msg,sizeof(msg),
                "Insufficient funds. Min balance Rs.%.2f must be maintained.",MIN_BALANCE);
            ui_error(msg); return;
        }
        if (client.dailyWithdrawn + amount > DAILY_LIMIT) {
            char msg[80];
            snprintf(msg,sizeof(msg),
                "Daily withdrawal limit Rs.%.2f exceeded. Used: Rs.%.2f",
                DAILY_LIMIT, client.dailyWithdrawn);
            ui_error(msg); return;
        }
        client.balance       -= amount;
        client.dailyWithdrawn += amount;
        client.totalSpent    += amount;
        client.monthlyWithdraw += amount;
        logTransaction(account, (op==3)?"ONLINE_TXN":"WITHDRAW", amount, client.balance);
        auditLog("WITHDRAW");
    } else {
        client.balance       += amount;
        client.monthlyDeposit += amount;
        logTransaction(account, "DEPOSIT", amount, client.balance);
        auditLog("DEPOSIT");
    }

    client.txnCount++;
    
    writeSlot(fPtr, account, &client);

    if (op != 1 && client.balance < MIN_BALANCE * 2)
        ui_warn("Suggestion: Balance is low. Consider a deposit.");

    ui_success("Transaction complete.");
    ui_tableHeader(); ui_tableRow(&client); ui_tableFooter();

    textFile(fPtr);
}

void newRecord(FILE *fPtr) {
    if (!sessionCheck()) return;
    if (session.role != ROLE_ADMIN) { ui_error("Admin access required."); return; }
    ui_header("ADD NEW ACCOUNT");

    struct clientData client = BLANK_CLIENT;
    unsigned int accountNum;

    printf("  Account number (1-%d): ", MAX_RECORDS);
    if (scanf("%u",&accountNum)!=1){clearInput();ui_error("Invalid.");return;}
    clearInput();
    if (accountNum<1||accountNum>MAX_RECORDS){ui_error("Out of range.");return;}

    struct clientData existing;
    if (readSlot(fPtr,accountNum,&existing) && existing.acctNum!=0) {
        ui_error("Account already exists."); return;
    }

    printf("  Last name   : "); scanf("%14s",client.lastName);  clearInput();
    printf("  First name  : "); scanf("%9s", client.firstName); clearInput();
    printf("  Balance     : "); scanf("%lf",&client.balance);   clearInput();
    printf("  PIN (6 digits): "); scanf("%6s",client.pin);      clearInput();
    printf("  Account type (1=Savings, 2=Current): ");
    int t; scanf("%d",&t); clearInput();
    client.accType = (t==2) ? ACC_CURRENT : ACC_SAVINGS;

    if (client.balance < MIN_BALANCE) {
        char msg[60];
        snprintf(msg,sizeof(msg),"Opening balance must be >= Rs.%.2f",MIN_BALANCE);
        ui_error(msg); return;
    }

    client.acctNum = accountNum;
    client.status  = STATUS_ACTIVE;
    getCurrentDate(client.lastTxnDate);
    getCurrentMonth(client.currentMonth);

    writeSlot(fPtr, accountNum, &client);
    logTransaction(accountNum,"ACCOUNT_OPEN",client.balance,client.balance);
    auditLog("NEW_ACCOUNT");

    appendToTextFile(&client);
    ui_success("Account created and appended to accounts.txt.");

    ui_tableHeader(); ui_tableRow(&client); ui_tableFooter();
}

void deleteRecord(FILE *fPtr) {
    if (!sessionCheck()) return;
    if (session.role != ROLE_ADMIN) { ui_error("Admin access required."); return; }
    ui_header("DELETE ACCOUNT");

    unsigned int accountNum;
    printf("  Account to delete (1-%d): ",MAX_RECORDS);
    if (scanf("%u",&accountNum)!=1){clearInput();ui_error("Invalid.");return;}
    clearInput();
    if (accountNum<1||accountNum>MAX_RECORDS){ui_error("Out of range.");return;}

    struct clientData client;
    if (!readSlot(fPtr,accountNum,&client)||client.acctNum==0) {
        ui_error("Account does not exist."); return;
    }

    ui_tableHeader(); ui_tableRow(&client); ui_tableFooter();

    int otp = generateOTP();
    if (!verifyOTP(otp)) { ui_error("OTP failed. Deletion cancelled."); return; }

    printf("  Confirm delete? (y/n): ");
    char ch[4]; scanf("%3s",ch); clearInput();
    if (tolower(ch[0])!='y') { ui_info("Cancelled."); return; }

    writeSlot(fPtr, accountNum, &BLANK_CLIENT);
    logTransaction(accountNum,"ACCOUNT_CLOSE",0,0);
    auditLog("DELETE_ACCOUNT");
    ui_success("Account deleted.");
    textFile(fPtr); 
}

void listAllRecords(FILE *fPtr) {
    if (!sessionCheck()) return;
    ui_header("ALL ACTIVE ACCOUNTS");
    struct clientData c; int count=0; double total=0;
    rewind(fPtr);
    ui_tableHeader();
    while (fread(&c,sizeof(struct clientData),1,fPtr)==1) {
        if (c.acctNum!=0) { ui_tableRow(&c); count++; total+=c.balance; }
    }
    ui_tableFooter();
    printf("  Total accounts: %d   Total balance: Rs.%.2f\n",count,total);
    auditLog("LIST_ALL");
}

void searchByName(FILE *fPtr) {
    if (!sessionCheck()) return;
    ui_header("SEARCH BY NAME");
    char q[15]; printf("  Enter last name: "); scanf("%14s",q); clearInput();
    char ql[15]; size_t i;
    for(i=0;i<strlen(q)+1;i++) ql[i]=(char)tolower((unsigned char)q[i]);

    struct clientData c; int found=0;
    rewind(fPtr);
    ui_tableHeader();
    while (fread(&c,sizeof(struct clientData),1,fPtr)==1) {
        if (c.acctNum==0) continue;
        char nl[15];
        for(i=0;i<strlen(c.lastName)+1;i++) nl[i]=(char)tolower((unsigned char)c.lastName[i]);
        if (strstr(nl,ql)) { ui_tableRow(&c); found++; }
    }
    ui_tableFooter();
    printf("  Found %d record(s).\n",found);
}

void accountSummary(FILE *fPtr) {
    if (!sessionCheck()) return;
    ui_header("ACCOUNT STATISTICS");
    struct clientData c;
    int cnt=0,sav=0,cur=0,frz=0,blk=0;
    double total=0,hi=-1e18,lo=1e18;
    char hiName[26]="N/A",loName[26]="N/A";
    rewind(fPtr);
    while(fread(&c,sizeof(struct clientData),1,fPtr)==1){
        if(c.acctNum==0) continue;
        cnt++; total+=c.balance;
        if(c.accType==ACC_SAVINGS) sav++; else cur++;
        if(c.status==STATUS_FROZEN)  frz++;
        if(c.status==STATUS_BLOCKED) blk++;
        if(c.balance>hi){hi=c.balance;snprintf(hiName,26,"%s %s",c.firstName,c.lastName);}
        if(c.balance<lo){lo=c.balance;snprintf(loName,26,"%s %s",c.firstName,c.lastName);}
    }
    if(cnt==0){ui_info("No active accounts.");return;}
    printf("  Active accounts  : %d (Savings:%d  Current:%d)\n",cnt,sav,cur);
    printf("  Frozen           : %d   Blocked: %d\n",frz,blk);
    printf("  Total balance    : Rs.%.2f\n",total);
    printf("  Average balance  : Rs.%.2f\n",total/cnt);
    printf("  Highest balance  : Rs.%.2f  (%s)\n",hi,hiName);
    printf("  Lowest  balance  : Rs.%.2f  (%s)\n",lo,loName);
    ui_thinline();
}

void transferFunds(FILE *fPtr) {
    if (!sessionCheck()) return;
    ui_header("TRANSFER FUNDS");

    unsigned int fromAcct,toAcct;
    double amount;
    printf("  From account: "); scanf("%u",&fromAcct); clearInput();
    if (!pinAuth(fPtr,fromAcct)) return;

    struct clientData from,to;
    if(!readSlot(fPtr,fromAcct,&from)||from.acctNum==0){ui_error("Source not found.");return;}

    printf("  To account  : "); scanf("%u",&toAcct); clearInput();
    if(fromAcct==toAcct){ui_error("Same account.");return;}
    if(!readSlot(fPtr,toAcct,&to)||to.acctNum==0){ui_error("Destination not found.");return;}

    /* beneficiary check (user mode) */
    if(session.role==ROLE_USER){
        int isBenef=0;
        char toStr[12]; snprintf(toStr,12,"%u",toAcct);
        for(int b=0;b<from.benefCount;b++)
            if(strcmp(from.beneficiaries[b],toStr)==0){isBenef=1;break;}
        if(!isBenef){ui_error("Destination not in beneficiary list. Add first.");return;}
    }

    printf("  Amount (Rs.): "); scanf("%lf",&amount); clearInput();
    if(amount<=0){ui_error("Invalid amount.");return;}

    resetDailyIfNeeded(&from);
    int fc=fraudCheck(&from,amount);
    if(fc==1){writeSlot(fPtr,fromAcct,&from);return;}

    /* OTP always for transfer */
    int otp=generateOTP();
    if(!verifyOTP(otp)){ui_error("OTP failed.");return;}

    if(from.balance-amount<MIN_BALANCE){
        char msg[80];
        snprintf(msg,sizeof(msg),"Insufficient. Min balance Rs.%.2f required.",MIN_BALANCE);
        ui_error(msg);return;
    }
    if(from.dailyWithdrawn+amount>DAILY_LIMIT){
        ui_error("Daily limit exceeded.");return;
    }

    from.balance         -= amount;
    from.dailyWithdrawn  += amount;
    from.totalSpent      += amount;
    from.monthlyWithdraw += amount;
    from.txnCount++;
    to.balance           += amount;
    to.monthlyDeposit    += amount;
    to.txnCount++;

    writeSlot(fPtr,fromAcct,&from);
    writeSlot(fPtr,toAcct,  &to);
    logTransaction(fromAcct,"TRANSFER_OUT",amount,from.balance);
    logTransaction(toAcct,  "TRANSFER_IN", amount,to.balance);
    auditLog("TRANSFER");

    char msg[80];
    snprintf(msg,sizeof(msg),"Rs.%.2f transferred from #%u to #%u",amount,fromAcct,toAcct);
    ui_success(msg);
    ui_tableHeader(); ui_tableRow(&from); ui_tableRow(&to); ui_tableFooter();
}

void applyInterest(FILE *fPtr) {
    if(!sessionCheck()) return;
    if(session.role!=ROLE_ADMIN){ui_error("Admin only.");return;}
    ui_header("APPLY INTEREST");
    printf("  Interest rate for Savings (default 4%%): ");
    double rate; scanf("%lf",&rate); clearInput();
    if(rate<=0||rate>20) rate=SAVINGS_INTEREST*100;
    rate/=100.0;

    struct clientData c; int cnt=0;
    rewind(fPtr);
    long pos=0;
    while(fread(&c,sizeof(struct clientData),1,fPtr)==1){
        if(c.acctNum!=0 && c.accType==ACC_SAVINGS && c.status==STATUS_ACTIVE){
            double interest=c.balance*rate;
            c.balance+=interest;
            c.monthlyDeposit+=interest;
            fseek(fPtr,pos,SEEK_SET);
            fwrite(&c,sizeof(struct clientData),1,fPtr);
            logTransaction(c.acctNum,"INTEREST",interest,c.balance);
            cnt++;
        }
        pos+=(long)sizeof(struct clientData);
    }
    char msg[60]; snprintf(msg,sizeof(msg),"Interest applied to %d savings accounts.",cnt);
    ui_success(msg);
    auditLog("INTEREST_APPLIED");
}

void freezeAccount(FILE *fPtr) {
    if(!sessionCheck()) return;
    if(session.role!=ROLE_ADMIN){ui_error("Admin only.");return;}
    ui_header("FREEZE / UNFREEZE ACCOUNT");
    unsigned int acct;
    printf("  Account number: "); scanf("%u",&acct); clearInput();
    struct clientData c;
    if(!readSlot(fPtr,acct,&c)||c.acctNum==0){ui_error("Not found.");return;}
    printf("  1=Freeze  2=Unfreeze  3=Block: "); int op; scanf("%d",&op); clearInput();
    if(op==1) c.status=STATUS_FROZEN;
    else if(op==2){c.status=STATUS_ACTIVE; c.pinAttempts=0;}
    else if(op==3) c.status=STATUS_BLOCKED;
    writeSlot(fPtr,acct,&c);
    auditLog("ACCOUNT_STATUS_CHANGED");
    ui_success("Status updated.");
    ui_tableHeader(); ui_tableRow(&c); ui_tableFooter();
}

void sortAccounts(FILE *fPtr) {
    if(!sessionCheck()) return;
    ui_header("SORT ACCOUNTS");
    printf("  Sort by: 1=Balance  2=Last Name: "); int by; scanf("%d",&by); clearInput();

    struct clientData arr[MAX_RECORDS];
    int n=0;
    rewind(fPtr);
    struct clientData tmp;
    while(fread(&tmp,sizeof(struct clientData),1,fPtr)==1)
        if(tmp.acctNum!=0) arr[n++]=tmp;

    for(int i=0;i<n-1;i++){
        for(int j=0;j<n-i-1;j++){
            int swap=0;
            if(by==2) swap=(strcmp(arr[j].lastName,arr[j+1].lastName)>0);
            else      swap=(arr[j].balance > arr[j+1].balance);
            if(swap){ struct clientData t=arr[j]; arr[j]=arr[j+1]; arr[j+1]=t; }
        }
    }
    ui_tableHeader();
    for(int i=0;i<n;i++) ui_tableRow(&arr[i]);
    ui_tableFooter();
    printf("  %d records sorted.\n",n);
}

void exportCSV(FILE *fPtr) {
    if(!sessionCheck()) return;
    FILE *csv=fopen(CSV_FILE,"w");
    if(!csv){ui_error("Cannot create accounts.csv.");return;}
    fprintf(csv,"AcctNum,LastName,FirstName,Balance,Type,Status,TotalSpent,TxnCount\n");
    struct clientData c; int cnt=0;
    rewind(fPtr);
    while(fread(&c,sizeof(struct clientData),1,fPtr)==1){
        if(c.acctNum==0) continue;
        const char *tp=(c.accType==ACC_SAVINGS)?"Savings":"Current";
        const char *st=(c.status==STATUS_ACTIVE)?"Active":
                       (c.status==STATUS_FROZEN)?"FROZEN":"BLOCKED";
        fprintf(csv,"%u,%s,%s,%.2f,%s,%s,%.2f,%d\n",
                c.acctNum,c.lastName,c.firstName,c.balance,tp,st,c.totalSpent,c.txnCount);
        cnt++;
    }
    fclose(csv);
    char msg[60]; snprintf(msg,sizeof(msg),"accounts.csv written — %d records.",cnt);
    ui_success(msg);
    auditLog("EXPORT_CSV");
}

void importFromCSV(FILE *fPtr) {
    if(!sessionCheck()) return;
    if(session.role!=ROLE_ADMIN){ui_error("Admin only.");return;}
    ui_header("IMPORT FROM CSV");
    char fname[64]; printf("  CSV filename: "); scanf("%63s",fname); clearInput();
    FILE *csv=fopen(fname,"r");
    if(!csv){ui_error("File not found.");return;}
    char line[200]; int cnt=0;
    fgets(line,sizeof(line),csv); /* skip header */
    while(fgets(line,sizeof(line),csv)){
        struct clientData c=BLANK_CLIENT;
        char type[10],status[10];
        if(sscanf(line,"%u,%14[^,],%9[^,],%lf,%9[^,],%9[^,]",
                  &c.acctNum,c.lastName,c.firstName,&c.balance,type,status)==6){
            c.accType=(strcmp(type,"Current")==0)?ACC_CURRENT:ACC_SAVINGS;
            c.status=(strcmp(status,"FROZEN")==0)?STATUS_FROZEN:
                     (strcmp(status,"BLOCKED")==0)?STATUS_BLOCKED:STATUS_ACTIVE;
            memcpy(c.pin,"000000",7);
            getCurrentDate(c.lastTxnDate);
            getCurrentMonth(c.currentMonth);
            writeSlot(fPtr,c.acctNum,&c);
            cnt++;
        }
    }
    fclose(csv);
    char msg[60]; snprintf(msg,sizeof(msg),"%d accounts imported.",cnt);
    ui_success(msg);
    auditLog("IMPORT_CSV");
}

void dailyReport(FILE *fPtr) {
    (void)fPtr;
    if(!sessionCheck()) return;
    ui_header("DAILY REPORT");
    char today[11]; getCurrentDate(today);
    printf("  Date: %s\n\n",today);
    FILE *tf=fopen(TXN_FILE,"r");
    if(!tf){ui_info("No transactions logged yet.");return;}
    char line[200]; int cnt=0; double total=0;
    printf("  Transactions today:\n");
    ui_thinline();
    while(fgets(line,sizeof(line),tf)){
        if(strncmp(line+1,today,10)==0){
            printf("  %s",line); cnt++;
            /* crude sum: extract amount field */
            char *p=strstr(line,"Amount:");
            if(p){ double a; if(sscanf(p+7,"%lf",&a)==1) total+=a; }
        }
    }
    fclose(tf);
    ui_thinline();
    printf("  Transactions today: %d   Total amount: Rs.%.2f\n",cnt,total);
}

void monthlySummary(FILE *fPtr) {
    if(!sessionCheck()) return;
    ui_header("MONTHLY SUMMARY");
    char month[8]; getCurrentMonth(month);
    printf("  Month: %s\n\n",month);
    struct clientData c; double totDep=0,totWd=0; int cnt=0;
    rewind(fPtr);
    while(fread(&c,sizeof(struct clientData),1,fPtr)==1){
        if(c.acctNum==0) continue;
        if(strcmp(c.currentMonth,month)==0){
            printf("  #%-4u %-15s Dep:Rs.%-10.2f Wd:Rs.%.2f\n",
                   c.acctNum,c.lastName,c.monthlyDeposit,c.monthlyWithdraw);
            totDep+=c.monthlyDeposit; totWd+=c.monthlyWithdraw; cnt++;
        }
    }
    ui_thinline();
    printf("  Accounts: %d  Total Deposits: Rs.%.2f  Total Withdrawals: Rs.%.2f\n",
           cnt,totDep,totWd);
}

void manageBeneficiaries(FILE *fPtr) {
    if(!sessionCheck()) return;
    ui_header("BENEFICIARY MANAGEMENT");
    unsigned int acct=session.acctNum;
    if(session.role==ROLE_ADMIN){
        printf("  Account number: "); scanf("%u",&acct); clearInput();
    }
    struct clientData c;
    if(!readSlot(fPtr,acct,&c)||c.acctNum==0){ui_error("Not found.");return;}

    printf("  Current beneficiaries:\n");
    for(int i=0;i<c.benefCount;i++) printf("   %d. %s\n",i+1,c.beneficiaries[i]);
    printf("  1=Add  2=Remove: "); int op; scanf("%d",&op); clearInput();

    if(op==1){
        if(c.benefCount>=MAX_BENEFICIARIES){ui_error("Max beneficiaries reached.");return;}
        printf("  Beneficiary account #: ");
        unsigned int ba; scanf("%u",&ba); clearInput();
        struct clientData bc;
        if(!readSlot(fPtr,ba,&bc)||bc.acctNum==0){ui_error("Account not found.");return;}
        char baStr[12]; snprintf(baStr,12,"%u",ba);
        strncpy(c.beneficiaries[c.benefCount],baStr,11);
        c.benefCount++;
        writeSlot(fPtr,acct,&c);
        ui_success("Beneficiary added.");
    } else if(op==2){
        printf("  Remove beneficiary # (1-%d): ",c.benefCount);
        int idx; scanf("%d",&idx); clearInput();
        if(idx<1||idx>c.benefCount){ui_error("Invalid.");return;}
        for(int i=idx-1;i<c.benefCount-1;i++)
            strncpy(c.beneficiaries[i],c.beneficiaries[i+1],11);
        c.benefCount--;
        writeSlot(fPtr,acct,&c);
        ui_success("Beneficiary removed.");
    }
    auditLog("BENEFICIARY_UPDATE");
}

void changePin(FILE *fPtr) {
    if(!sessionCheck()) return;
    ui_header("CHANGE PIN");
    unsigned int acct=session.acctNum;
    if(session.role==ROLE_ADMIN){
        printf("  Account number: "); scanf("%u",&acct); clearInput();
    }
    if(!pinAuth(fPtr,acct)) return;
    struct clientData c;
    readSlot(fPtr,acct,&c);
    printf("  New PIN (6 digits): "); scanf("%6s",c.pin); clearInput();
    writeSlot(fPtr,acct,&c);
    auditLog("PIN_CHANGED");
    ui_success("PIN updated successfully.");
}

void checkDataIntegrity(FILE *fPtr) {
    if(!sessionCheck()) return;
    ui_header("DATA INTEGRITY CHECK");
    fseek(fPtr,0,SEEK_END);
    long sz=ftell(fPtr);
    long expected=(long)MAX_RECORDS*(long)sizeof(struct clientData);
    if(sz!=expected){
        char msg[80];
        snprintf(msg,sizeof(msg),"File size mismatch! Got %ld bytes, expected %ld.",sz,expected);
        ui_error(msg);
        ui_warn("Run 'Restore from backup' to fix.");
        return;
    }
    
    struct clientData c; int bad=0; rewind(fPtr);
    while(fread(&c,sizeof(struct clientData),1,fPtr)==1){
        if(c.acctNum>MAX_RECORDS) bad++;
        if(c.balance<-1e9) bad++;
    }
    if(bad>0){
        char msg[60]; snprintf(msg,sizeof(msg),"%d suspicious records found.",bad);
        ui_warn(msg);
    } else {
        ui_success("Data integrity OK — no corruption detected.");
    }
    char info[60]; snprintf(info,sizeof(info),"File size: %ld bytes (expected %ld)",sz,expected);
    ui_info(info);
    auditLog("INTEGRITY_CHECK");
}

void backupData(void) {
    FILE *src=fopen(DATA_FILE,"rb");
    if(!src) return;
    FILE *dst=fopen(BACKUP_FILE,"wb");
    if(!dst){fclose(src);return;}
    char buf[4096]; size_t n;
    while((n=fread(buf,1,sizeof(buf),src))>0) fwrite(buf,1,n,dst);
    fclose(src); fclose(dst);
    ui_success("Database backed up to backup.dat");
    auditLog("BACKUP");
}

void restoreData(FILE **fPtr) {
    ui_header("RESTORE FROM BACKUP");
    printf("  Confirm restore? This will overwrite current data. (y/n): ");
    char ch[4]; scanf("%3s",ch); clearInput();
    if(tolower(ch[0])!='y'){ui_info("Cancelled.");return;}
    fclose(*fPtr);
    FILE *src=fopen(BACKUP_FILE,"rb");
    if(!src){ui_error("Backup file not found.");
        *fPtr=fopen(DATA_FILE,"rb+"); return;}
    FILE *dst=fopen(DATA_FILE,"wb");
    char buf[4096]; size_t n;
    while((n=fread(buf,1,sizeof(buf),src))>0) fwrite(buf,1,n,dst);
    fclose(src); fclose(dst);
    *fPtr=fopen(DATA_FILE,"rb+");
    auditLog("RESTORE");
    ui_success("Data restored from backup.");
}

void processQueue(FILE *fPtr) {
    if(!sessionCheck()) return;
    ui_header("QUEUED TRANSACTION SYSTEM");
    printf("  1=Add to queue  2=Process queue: "); int op; scanf("%d",&op); clearInput();
    if(op==1){
        struct TxnRequest *req=(struct TxnRequest*)malloc(sizeof(struct TxnRequest));
        if(!req){ui_error("Memory error.");return;}
        printf("  Type (DEPOSIT/WITHDRAW/TRANSFER): "); scanf("%19s",req->type); clearInput();
        printf("  Account #: "); scanf("%u",&req->acct); clearInput();
        printf("  Amount   : "); scanf("%lf",&req->amount); clearInput();
        req->toAcct=0;
        if(strcmp(req->type,"TRANSFER")==0){
            printf("  To account #: "); scanf("%u",&req->toAcct); clearInput();
        }
        req->next=NULL;
        if(!txnQueueHead) txnQueueHead=txnQueueTail=req;
        else { txnQueueTail->next=req; txnQueueTail=req; }
        ui_success("Transaction queued.");
    } else {
        int processed=0;
        while(txnQueueHead){
            struct TxnRequest *req=txnQueueHead;
            printf("  Processing: %s Acct#%u Rs.%.2f\n",req->type,req->acct,req->amount);
            struct clientData c;
            if(readSlot(fPtr,req->acct,&c)&&c.acctNum!=0){
                if(strcmp(req->type,"DEPOSIT")==0){
                    c.balance+=req->amount;
                    logTransaction(req->acct,"QUEUED_DEP",req->amount,c.balance);
                } else if(strcmp(req->type,"WITHDRAW")==0){
                    if(c.balance-req->amount>=MIN_BALANCE){
                        c.balance-=req->amount;
                        logTransaction(req->acct,"QUEUED_WD",req->amount,c.balance);
                    } else ui_warn("Skipped: insufficient balance.");
                } else if(strcmp(req->type,"TRANSFER")==0){
                    struct clientData t;
                    if(readSlot(fPtr,req->toAcct,&t)&&t.acctNum!=0
                       &&c.balance-req->amount>=MIN_BALANCE){
                        c.balance-=req->amount; t.balance+=req->amount;
                        writeSlot(fPtr,req->toAcct,&t);
                        logTransaction(req->acct,"QUEUED_TXFR",req->amount,c.balance);
                    }
                }
                writeSlot(fPtr,req->acct,&c);
                processed++;
            }
            txnQueueHead=req->next; free(req);
        }
        txnQueueTail=NULL;
        char msg[60]; snprintf(msg,sizeof(msg),"%d queued transactions processed.",processed);
        ui_success(msg);
    }
}

void spendingAnalysis(FILE *fPtr) {
    if(!sessionCheck()) return;
    ui_header("SPENDING PATTERN ANALYSIS");
    unsigned int acct=session.acctNum;
    if(session.role==ROLE_ADMIN){
        printf("  Account #: "); scanf("%u",&acct); clearInput();
    }
    struct clientData c;
    if(!readSlot(fPtr,acct,&c)||c.acctNum==0){ui_error("Not found.");return;}
    printf("  Account   : #%u %s %s\n",c.acctNum,c.firstName,c.lastName);
    printf("  Total spent (lifetime) : Rs.%.2f\n",c.totalSpent);
    printf("  Total transactions     : %d\n",c.txnCount);
    printf("  This month deposits    : Rs.%.2f\n",c.monthlyDeposit);
    printf("  This month withdrawals : Rs.%.2f\n",c.monthlyWithdraw);
    if(c.txnCount>0)
        printf("  Avg transaction amount : Rs.%.2f\n",c.totalSpent/c.txnCount);
    ui_thinline();
    
    printf("  Recent transactions (from log):\n");
    FILE *tf=fopen(TXN_FILE,"r");
    if(tf){
        char lines[20][200]; int lcount=0;
        char acctStr[12]; snprintf(acctStr,12,"#%-4u",acct);
        char line[200];
        while(fgets(line,sizeof(line),tf)){
            if(strstr(line,acctStr)){
                strncpy(lines[lcount%10],line,199); lcount++;
            }
        }
        fclose(tf);
        int start=(lcount>10)?lcount-10:0;
        for(int i=start;i<lcount&&i<start+10;i++)
            printf("  %s",lines[i%10]);
    }
}

void showSmartSuggestions(FILE *fPtr) {
    if(!sessionCheck()) return;
    ui_header("SMART SUGGESTIONS");
    unsigned int acct=session.acctNum;
    struct clientData c;
    if(!readSlot(fPtr,acct,&c)||c.acctNum==0){ui_error("Not found.");return;}
    int suggestions=0;
    if(c.balance<MIN_BALANCE*2){
        ui_warn("Balance is low. Consider a deposit to maintain buffer.");suggestions++;}
    if(c.dailyWithdrawn>DAILY_LIMIT*0.8){
        ui_warn("You are close to your daily withdrawal limit.");suggestions++;}
    if(c.monthlyWithdraw>c.monthlyDeposit*0.9 && c.monthlyDeposit>0){
        ui_warn("Spending is close to income this month. Review expenses.");suggestions++;}
    if(c.accType==ACC_SAVINGS && c.balance>50000){
        ui_info("High balance in savings. Consider investing for better returns.");suggestions++;}
    if(c.txnCount>20){
        ui_info("Frequent transactions detected. Consider using NEFT for bulk payments.");suggestions++;}
    if(suggestions==0) ui_success("No suggestions — your account looks healthy!");
}

void adminPanel(FILE *fPtr) {
    if(!sessionCheck()) return;
    if(session.role!=ROLE_ADMIN){ui_error("Admin only.");return;}
    ui_header("ADMIN PANEL");
    printf("  1=View audit log  2=View all transactions  3=Force unblock account\n  Choice: ");
    int op; scanf("%d",&op); clearInput();
    if(op==1){
        FILE *af=fopen(AUDIT_FILE,"r");
        if(!af){ui_info("Audit log empty.");return;}
        char line[200]; int i=0;
        printf("\n  Last 30 audit entries:\n"); ui_thinline();
        while(fgets(line,sizeof(line),af)&&i++<30) printf("  %s",line);
        fclose(af);
    } else if(op==2){
        FILE *tf=fopen(TXN_FILE,"r");
        if(!tf){ui_info("No transactions.");return;}
        char line[200]; int i=0;
        printf("\n  Last 30 transactions:\n"); ui_thinline();
        while(fgets(line,sizeof(line),tf)&&i++<30) printf("  %s",line);
        fclose(tf);
    } else if(op==3){
        printf("  Account to unblock: "); unsigned int a; scanf("%u",&a); clearInput();
        struct clientData c;
        if(!readSlot(fPtr,a,&c)||c.acctNum==0){ui_error("Not found.");return;}
        c.status=STATUS_ACTIVE; c.pinAttempts=0;
        writeSlot(fPtr,a,&c);
        auditLog("ADMIN_UNBLOCK");
        ui_success("Account unblocked.");
    }
}

