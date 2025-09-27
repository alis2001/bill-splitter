import React from 'react';
import { View, Text, TouchableOpacity, StyleSheet } from 'react-native';
import { Ionicons } from '@expo/vector-icons';

const EventCard = ({ event, onPress, userBalance = 0, participantCount = 0 }) => {
  const getStatusColor = () => {
    if (event.status === 'completed') return '#38A169';
    if (userBalance > 0) return '#38A169';
    if (userBalance < 0) return '#E53E3E';
    return '#A0AEC0';
  };

  const getStatusText = () => {
    if (event.status === 'completed') return 'Settled';
    if (userBalance > 0) return `+$${Math.abs(userBalance).toFixed(2)} owed`;
    if (userBalance < 0) return `$${Math.abs(userBalance).toFixed(2)} you owe`;
    return 'Even';
  };

  const getEventIcon = () => {
    switch (event.event_type) {
      case 'restaurant': return 'restaurant';
      case 'travel': return 'airplane';
      case 'shopping': return 'bag';
      case 'entertainment': return 'film';
      case 'utilities': return 'home';
      default: return 'receipt';
    }
  };

  return (
    <TouchableOpacity style={styles.card} onPress={onPress} activeOpacity={0.7}>
      <View style={styles.header}>
        <View style={styles.iconContainer}>
          <Ionicons 
            name={getEventIcon()} 
            size={24} 
            color="#1A365D" 
          />
        </View>
        <View style={styles.content}>
          <Text style={styles.title} numberOfLines={1}>
            {event.name}
          </Text>
          <Text style={styles.subtitle}>
            {participantCount} {participantCount === 1 ? 'person' : 'people'} • {event.event_type}
          </Text>
        </View>
        <View style={styles.statusContainer}>
          <Text style={[styles.statusText, { color: getStatusColor() }]}>
            {getStatusText()}
          </Text>
          <Ionicons name="chevron-forward" size={16} color="#A0AEC0" />
        </View>
      </View>
    </TouchableOpacity>
  );
};

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#FFFFFF',
    borderRadius: 12,
    padding: 16,
    marginHorizontal: 16,
    marginVertical: 6,
    shadowColor: '#000',
    shadowOffset: {
      width: 0,
      height: 2,
    },
    shadowOpacity: 0.05,
    shadowRadius: 3,
    elevation: 2,
  },
  header: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  iconContainer: {
    width: 48,
    height: 48,
    borderRadius: 24,
    backgroundColor: '#E6FFFA',
    justifyContent: 'center',
    alignItems: 'center',
    marginRight: 12,
  },
  content: {
    flex: 1,
  },
  title: {
    fontSize: 16,
    fontWeight: '600',
    color: '#2D3748',
    marginBottom: 2,
  },
  subtitle: {
    fontSize: 14,
    color: '#718096',
  },
  statusContainer: {
    alignItems: 'flex-end',
  },
  statusText: {
    fontSize: 14,
    fontWeight: '500',
    marginBottom: 2,
  },
});

export default EventCard;