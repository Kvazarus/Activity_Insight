# Activity Insight

[Русский](#ru) | [English](#en)

---

## <a id="ru"></a>Русский

**Activity Insight** — это настольное приложение для ОС Windows, предназначенное для автоматизированного сбора и глубокого анализа статистики использования программного обеспечения. Проект ориентирован на пользователей, которые ценят приватность и предпочитают локальное хранение данных без передачи телеметрии на внешние серверы.

### Основные функции
* **Фоновый мониторинг**: Автоматическое отслеживание активных окон и определение исполняемых файлов через WinAPI.
* **Умное определение простоя**: Система распознает отсутствие активности пользователя, учитывая исключения для полноэкранных приложений (игры, видео).
* **Гибкая классификация**: Группировка программ по категориям («Работа», «Учёба», «Развлечения») с возможностью настройки цветов.
* **Визуализация статистики**: Интерактивные круговые диаграммы и гистограммы динамики за неделю.
* **Режим фокусировки**: Встроенный Pomodoro-таймер с системой уведомлений об отвлечениях.
* **Приватность**: Полная Offline-first архитектура; база данных SQLite хранится только на вашем ПК.

### Технологический стек
* **Язык**: C++ (стандарт 17/20).
* **Фреймворк**: Qt 6 (QtWidgets, QtCharts, QtSql).
* **База данных**: SQLite.
* **Системное API**: Windows API (User32, Shell32, Psapi).

### Системные требования
* **ОС**: Windows 10 / 11 (64-bit).
* **Архитектура**: x64.
* **Библиотеки**: Microsoft Visual C++ Redistributable.

---

## <a id="en"></a>English

**Activity Insight** is a Windows desktop application designed for automated collection and in-depth analysis of software usage statistics. The project is tailored for users who prioritize privacy and prefer local data storage without transmitting telemetry to external servers.

### Key Features
* **Background Monitoring**: Automatic tracking of active windows and executable file identification via WinAPI.
* **Smart IDLE Detection**: Recognizes periods of user inactivity while accounting for full-screen exceptions (games, video).
* **Flexible Categorization**: Group applications into categories (Work, Learning, Entertainment) with customizable color coding.
* **Data Visualization**: Interactive pie charts and weekly activity bar charts.
* **Focus Mode**: Built-in Pomodoro timer with a notification system to prevent distractions.
* **Privacy-Focused**: Complete Offline-first architecture; the SQLite database is stored exclusively on your PC.

### Tech Stack
* **Language**: C++ (17/20 standard).
* **Framework**: Qt 6 (QtWidgets, QtCharts, QtSql).
* **Database**: SQLite.
* **System API**: Windows API (User32, Shell32, Psapi).

### System Requirements
* **OS**: Windows 10 / 11 (64-bit).
* **Architecture**: x64.
* **Libraries**: Microsoft Visual C++ Redistributable.

---

## License / Лицензия
This application uses the Qt framework (https://www.qt.io). Qt is licensed under the GNU GPL v3. 

Данное приложение использует фреймворк Qt (https://www.qt.io). Qt распространяется под лицензией GNU GPL v3.
