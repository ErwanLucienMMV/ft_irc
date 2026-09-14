# Parcours de tests pour la soutenance

Prévoir trois clients Irssi : **alice**, **bob** et **carol**.
Les commandes commençant par `/` se tapent dans Irssi ; les commandes Bash
se lancent dans un terminal. Suivre les étapes dans l'ordre.

Pour chaque test, vérifier le résultat chez l'émetteur, les destinataires
et un client qui ne doit rien recevoir.

## 1. Lancement et connexion

Dans le terminal du serveur :

```bash
./ircserv 6667 secret
```

Dans trois autres terminaux :

```bash
irssi -c 127.0.0.1 -p 6667 -w secret -n alice
irssi -c 127.0.0.1 -p 6667 -w secret -n bob
irssi -c 127.0.0.1 -p 6667 -w secret -n carol
```

Tester également une connexion avec un mauvais mot de passe : elle ne doit
pas permettre de rejoindre un salon.

Le lancement sans mot de passe reste possible pour les tests locaux :

```bash
./ircserv 6667
irssi -c 127.0.0.1 -p 6667 -n alice
```

Arrêter le premier serveur avant de réutiliser le même port.

## 2. Salons et messages

| Client | Commande | Résultat attendu |
|---|---|---|
| alice | `/join #test` | Création du salon ; Alice est opératrice (`@`). |
| bob | `/join #test` | Bob rejoint ; Alice voit son arrivée. |
| alice | `/msg #test Bonjour tout le monde` | Bob reçoit le message. |
| bob | `/msg alice Salut en privé` | Alice reçoit le message privé ; Carol ne reçoit rien. |
| carol | `/msg #test Je suis dehors` | Refus : Carol n'est pas membre. |
| alice | `/join #autre,#troisieme` | Alice rejoint les deux salons. |

## 3. Pseudos

Sur Bob :

```irc
/nick alice
/nick 123invalide
/nick robert
/nick bob
```

Les deux premières commandes doivent être refusées. Les changements valides
doivent apparaître chez Alice.

## 4. Sujet du salon : TOPIC et mode t

| Client | Commande | Résultat attendu |
|---|---|---|
| alice | `/mode #test +t` | Seuls les opérateurs peuvent modifier le sujet. |
| bob | `/topic #test Sujet interdit` | Refus. |
| alice | `/topic #test Bienvenue` | Sujet modifié, visible chez Bob. |
| bob | `/topic #test` | Affiche le sujet. |
| alice | `/mode #test -t` | Modification ouverte aux membres. |
| bob | `/topic #test Sujet de Bob` | Réussite. |
| bob | `/quote TOPIC #test :` | Efface le sujet. |

## 5. Invitations : mode i et INVITE

| Client | Commande | Résultat attendu |
|---|---|---|
| alice | `/mode #test +i` | Salon accessible sur invitation. |
| carol | `/join #test` | Refus. |
| bob | `/invite carol #test` | Refus : Bob n'est pas opérateur. |
| alice | `/invite carol #test` | Carol reçoit l'invitation. |
| carol | `/join #test` | Réussite. |
| carol | `/part #test` | Carol quitte le salon. |
| alice | `/mode #test -i` | Retrait de la restriction. |

## 6. Clé du salon : mode k

| Client | Commande | Résultat attendu |
|---|---|---|
| alice | `/mode #test +k cle` | Protection par clé. |
| carol | `/mode #test` | La vraie clé ne doit pas être révélée. |
| carol | `/join #test` | Refus. |
| carol | `/join #test mauvaise` | Refus. |
| carol | `/join #test cle` | Réussite. |
| carol | `/part #test` | Carol quitte. |
| alice | `/mode #test -k cle` | Retrait de la clé. |

## 7. Limite de membres : mode l

Alice et Bob doivent être les deux seuls membres de `#test`.

| Client | Commande | Résultat attendu |
|---|---|---|
| alice | `/mode #test +l 2` | Limite fixée à deux membres. |
| carol | `/join #test` | Refus : salon plein. |
| alice | `/mode #test -l` | Suppression de la limite. |
| carol | `/join #test` | Réussite. |

## 8. Opérateurs : mode o et KICK

| Client | Commande | Résultat attendu |
|---|---|---|
| bob | `/kick #test carol essai` | Refus : Bob n'est pas opérateur. |
| alice | `/mode #test +o bob` | Bob devient opérateur. |
| bob | `/kick #test carol dehors` | Carol est expulsée ; les membres en sont informés. |
| carol | `/msg #test encore ici ?` | Refus. |
| alice | `/mode #test -o bob` | Bob perd ses droits. |
| bob | `/mode #test +i` | Refus. |

## 9. Régressions MODE

Sur Alice :

```irc
/mode #test +k cle
/quote MODE #test -k+o cle bob
```

La clé doit disparaître et Bob devenir opérateur.

Puis :

```irc
/quote MODE #test +ik
/mode #test
```

La première commande doit être refusée **sans activer +i**.

Enfin :

```irc
/quote MODE #test +k :deux mots
```

La clé invalide doit être refusée.

## 10. Plusieurs salons avec des clés

Sur Alice :

```irc
/join #public,#prive
/mode #prive +k secret
```

Sur Carol :

```irc
/quote JOIN #public,#prive ,secret
```

Elle doit rejoindre les deux salons. Pour vérifier qu'un refus ne bloque
pas les suivants, quitter d'abord le salon protégé :

```irc
/part #prive
/quote JOIN #prive,#nouveau mauvaise
```

Carol doit être refusée dans `#prive`, mais rejoindre `#nouveau`.

## 11. Déconnexions et nettoyage

- Bob fait `/quit` : Alice voit son départ et peut encore discuter avec un autre client.
- Fermer brutalement un client : les autres doivent continuer à fonctionner.
- Sur Alice, créer un salon inutilisé avec `/join #nettoyage`, puis faire
  `/kick #nettoyage alice`. Carol fait ensuite `/join #nettoyage` : elle doit
  devenir opératrice du salon recréé.
- Arrêter le serveur avec `Ctrl+C`, puis le relancer : aucun ancien salon
  ni utilisateur ne doit subsister.

## 12. Commande reçue en plusieurs morceaux

Dans un terminal :

```bash
nc -C 127.0.0.1 6667
```

Taper `PI`, appuyer sur **Ctrl+D**, puis taper `NG :fragment` et appuyer
sur **Entrée**. Le serveur doit attendre la fin de la ligne et répondre :

```irc
PONG :fragment
```

Si la variante locale de `nc` ne reconnaît pas `-C`, utiliser `nc` sans cette
option : ce serveur accepte également les lignes terminées par LF.

Ce document est un parcours manuel de vérification, pas une garantie
d'absence de bugs. Il reste local dans `docs/`, ignoré par Git.
