/* version and history */

#ifndef XDAG_VERSION_H
#define XDAG_VERSION_H

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

#define VERSION_MAJOR               0
#define VERSION_MINOR               5
#define VERSION_REVISION            0

#define XDAG_VERSION STRINGIZE(VERSION_MAJOR) "." STRINGIZE(VERSION_MINOR) "." STRINGIZE(VERSION_REVISION)


/* version history
0.5.0 RandomX POW

0.4.0 rocksdb store

0.3.1 apollo feature hark fork

0.2.3 bug with payments is fixed

0.2.2 pool logic is refactored
      verifying of shares from miner
      initial version of JSON-RPC
*/

//version history in russian
/* история
T13.895 более аккуратное сокращение времени прохождения транзакции (132 сек максимум);
	статистика теперь усредняется по 4-часовому интервалу; более аккуратное
	устранение утечки памяти; исправлены падения по сигналам 15, 28

T13.889 время подтверждения транзакции сокращено на минуту, устранена утечка памяти,
	исправлена ошибка с падением при переинициализации системы блоков

T13.878 исправлена ошибка с расхождением числа главных блоков

T13.865 исправлена ошибка с перечислением дробных сумм

T13.856 добавлена команда block, показывающая информацию о блоке

T13.853 коммуникация через fifo заменена на сокеты unix

T13.852 исправлена ошибка со слишком большим дескриптором сокета в select

T13.847 игнорирование сигнала SIGWINCH, слияние с windows

T13.845 ещё одно улучшение производительности sha256 с использованием sse/avx/avx2

T13.842 улучшение производительности sha256

T13.841 исправление ошибок, сборка для Windows

T13.838 добавлена опция -a, позволяющая явно задавать адрес, используемый в майнере;
	изменён способ подсчёта хешрейта и способ распределения выигрыша среди майнеров

T13.830 добавлена опция -v, задающая начальный уровень логирования; исправлена ошибка,
	приводящая к падению с сигналом 6

T13.826 добавлена опция -l, позволяющая выводить ненулевой баланс всех адресов в системе

T13.819 добавлен подсчёт собственного хешрейта майнера

T13.818 обновление для пулов: 1) блоки вынесены в файл, отображаемый в память;
	2) новый пул можно запускать параллельно с работающим старым, он будет 
	подгружать локальные блоки; 3) исправлена ошибка с порчей итоговых хешей 
	примерно в половине случаев; 4) теперь можно перечислять pool fee, которых
	не находятся в последних 32 блоках; 5) теперь добавлять новые пулы в белый 
	список можно без перезагрузки старых; 6) пожертвования в фонд сообщества.

T13.811 исправлена ошибка с появлением другого адреса в майнере, токен изменен на XDAG

T13.808 удалён неиспользуемый код

T13.806 исправлены ошибки: иногда транзакции не проходят, суммы меньше 1 читкоина
	проходят некорректно; добавлен графический кошелёк для виндовз

T13.798 в пуле добавлено ограничение числа майнеров с одного ip; исправлена ошибка
	с отменой транзакции при неправильном адресе одного из майнеров

T13.793 исправлена ошибка с неправильным определением правильности пароля

T13.790 исправлена утечка памяти и программа не должна падать

T13.789 сделан пароль на кошелёк, добавлена команда state, сделана перезагрузка
	системы обратотки блоков в случае порчи локального хранилища блоков

T13.785 сделана блокировка входящих соединений от хостов, от которых уже есть >= 4
	соединения

T13.781 устранена утечка памяти, добавлена команда net connlimit

T13.778 внесены изменения для windows

T13.776 сделаны пул и майнер с функцией перевода денег (лёгкая нода)

T13.764 улучшения в синхронизации

T13.761 сделан быстрый механизм перебора хешей в майнере, дающий выгоду x2-x3

T13.760 добавлен вывод своего и обзего хешрейта сети за прошедший час

T13.759 Linux: при падении в лог добавляется стек вызовов, программа компилируется 
	с NDEBUG, отключены fifo-мьютексы dthread

T13.753 увеличен период, за котоырй запрашиваются все блоки, сделана версия для win64

T13.748 добавлен код, позволяющий избежать падений из-за отключения соединений

T13.744 исправлена ошибка в алгоритме майнинга

T13.742 улучшение алгоритма синхронизации, улучшение алгоритма майнинга,
	теперь команда net hosts будет выводить версии cheatcoin, а не dnet

T13.740 код для Windows и Linux объединён, новый алгоритм синхронизации хостов

T13.734 иправлена ошибка с неверным определением позиции в файле при записи блока;
	при ответе на зпаросов блока за период в минуту не создаётся новый поток;
	программа завершается, если нет базы данных хостов

T13.726 исправлена ошибка, когда вновь созданный блок отвергается; сделана команда
	terminate для завершения демона

T13.720 добавлены генерация новых ключей и транзакции с разными ключами, README.md

T13.718 исправлены ошибки, связанные с размножением пакетов в сети

T13.716 сделана база данных хостов

T13.714 изменён механизм синхронизации между хостами с использованием контрольных сумм,
	сделана команда net ... для вызова команды транспортного уровня

T13.707 расширен набор команд, сделаны переводы на данный адрес

T13.706 сделано включение неглавных блоков в общую сеть

T13.702 изменён алгоритм назначения главных блоков (блоки из цепи с наибольшей
	суммарной сложностью), доделаны применение и откатка транзакций

T13.696 скорость синхронизации устанавливается пропорционально доле неизвестных главных
	блоков; добавлены уровни логирования и их интерактивное изменение

T13.695 хранилище сделано в виде структуры каталогов; добавлена опция -i,
	позволяющая запустить программу в качестве терминала к работающему демону

T13.693 исправлена ошибка в storage; запущена тестовая сеть

T13.692 изменён алгоритм отпределения главного блока, добавлены адреса в формате base64

T13.690 криптография отлажена

T13.682 добавлено кодирование ECDSA и кошелёк wallet.dat

T13.673 работающая версия на сети из двух хостов

T13.672 первая рабочая версия: интерактивное меню, майнинг, хеш, лог-файл,
	сохранение блоков в файл и обмен между хостами

T13.654	начало проекта

*/

#endif
