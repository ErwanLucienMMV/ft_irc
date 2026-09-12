# Organisation du serveur

Le programme utilise une classe `Server` et une structure `Client`.
En C++, une structure peut avoir un constructeur et des méthodes comme une
classe. Ici, `Client` expose simplement les données d'une connexion.

## Fichiers

- `srcs/main.cpp` : valide les arguments, crée un `Server` et le lance.
- `includes/server.hpp` : décrit les données et les méthodes de `Server`.
- `srcs/server.cpp` : contient l'implémentation du serveur.
- `includes/client.hpp` : décrit une connexion cliente.

## Server

Un serveur conserve le port, le mot de passe, le socket d'écoute, l'ensemble
des sockets surveillés et les clients connectés.

`std::map<int, Client> _clients` associe chaque descripteur à un client.
Par exemple, la clé `4` permet de retrouver les informations de la connexion
dont le socket est `4`. Les clients sont stockés par valeur, sans `new` ni `delete`.

Les méthodes publiques sont celles utilisées par `main()` :

1. Le constructeur prépare les données, sans ouvrir de socket.
2. `start()` ouvre le socket et prépare l'écoute. Elle retourne `false` en cas d'échec.
3. `run()` exécute la boucle `select()`.
4. Le destructeur ferme les sockets encore ouverts lorsque l'objet est détruit.

Les méthodes privées réalisent les opérations internes : création du socket,
acceptation, réception, traitement du message et déconnexion.
Leur accès aux membres de `Server` évite de passer `master` et `max_fd` en paramètres.

Le serveur possède les sockets : lui seul les ferme. Il ne peut pas être copié,
car deux copies risqueraient de fermer les mêmes descripteurs. Les déclarations
privées du constructeur de copie et de l'opérateur d'affectation imposent cette
règle en C++98.

## Client

Un client conserve actuellement son descripteur, son adresse IP et son port distant.
Il décrit la connexion mais ne ferme pas lui-même le socket. Copier ses données
dans la map ne crée donc pas un second propriétaire du socket.

## Prochaines évolutions

Le traitement existant de `CAP LS` est isolé dans `handleMessage()`. Il recherche
encore du texte dans chaque bloc reçu : ce n'est pas encore un parseur de lignes
ni une inscription IRC complète. Le mot de passe est conservé mais pas encore vérifié.

La suite pourra ajouter progressivement :

1. Le mode non bloquant, les limites de `select()` et la gestion de l'arrêt.
2. Des tampons de réception et d'envoi dans `Client`.
3. Un parseur qui reçoit une ligne complète et en extrait la commande et les paramètres.
4. Le pseudo, le nom d'utilisateur et l'état d'inscription dans `Client`.
5. Un type `Channel` lorsque les commandes de canaux seront implémentées.

Les sockets restent actuellement bloquants et les réponses sont encore envoyées
directement. Le destructeur assure le nettoyage lors d'une sortie normale de portée
ou d'une exception ; il ne remplace pas un gestionnaire d'arrêt par signal.
