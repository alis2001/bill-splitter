import React, { useState } from 'react';
import { View, Text, StyleSheet, ScrollView, Alert, TouchableOpacity } from 'react-native';
import { SafeAreaView } from 'react-native-safe-area-context';
import { TextInput, Button } from 'react-native-paper';
import { Ionicons } from '@expo/vector-icons';
import { eventsAPI } from '../../services/apiService';

const EVENT_TYPES = [
  { key: 'restaurant', label: 'Restaurant', icon: 'restaurant', color: '#FF6B6B' },
  { key: 'travel', label: 'Travel', icon: 'airplane', color: '#4ECDC4' },
  { key: 'shopping', label: 'Shopping', icon: 'bag', color: '#45B7D1' },
  { key: 'entertainment', label: 'Fun', icon: 'film', color: '#96CEB4' },
  { key: 'utilities', label: 'Bills', icon: 'home', color: '#FFEAA7' },
  { key: 'other', label: 'Other', icon: 'receipt', color: '#DDA0DD' },
];

const CreateEventScreen = ({ navigation }) => {
  const [name, setName] = useState('');
  const [description, setDescription] = useState('');
  const [selectedType, setSelectedType] = useState('restaurant');
  const [loading, setLoading] = useState(false);

  const handleCreate = async () => {
    if (!name.trim()) {
      Alert.alert('Oops!', 'Give your event a name');
      return;
    }

    setLoading(true);
    const result = await eventsAPI.createEvent({
      name: name.trim(),
      description: description.trim(),
      event_type: selectedType,
    });

    setLoading(false);
    
    if (result.success) {
      navigation.goBack();
    } else {
      Alert.alert('Error', result.error);
    }
  };

  return (
    <SafeAreaView style={styles.container}>
      <View style={styles.header}>
        <TouchableOpacity onPress={() => navigation.goBack()}>
          <Ionicons name="close" size={24} color="#2D3748" />
        </TouchableOpacity>
        <Text style={styles.title}>New Event</Text>
        <View style={{ width: 24 }} />
      </View>

      <ScrollView style={styles.content} showsVerticalScrollIndicator={false}>
        <TextInput
          label="Event name"
          value={name}
          onChangeText={setName}
          mode="outlined"
          style={styles.input}
          placeholder="e.g., Dinner with friends"
          autoFocus
        />

        <TextInput
          label="Description (optional)"
          value={description}
          onChangeText={setDescription}
          mode="outlined"
          multiline
          numberOfLines={2}
          style={styles.input}
          placeholder="Add some details..."
        />

        <Text style={styles.sectionTitle}>What type of event?</Text>
        <View style={styles.typeGrid}>
          {EVENT_TYPES.map((type) => (
            <TouchableOpacity
              key={type.key}
              style={[
                styles.typeCard,
                selectedType === type.key && { 
                  backgroundColor: type.color,
                  transform: [{ scale: 1.05 }]
                }
              ]}
              onPress={() => setSelectedType(type.key)}
              activeOpacity={0.7}
            >
              <Ionicons 
                name={type.icon} 
                size={24} 
                color={selectedType === type.key ? '#FFFFFF' : type.color} 
              />
              <Text style={[
                styles.typeLabel,
                selectedType === type.key && { color: '#FFFFFF', fontWeight: '600' }
              ]}>
                {type.label}
              </Text>
            </TouchableOpacity>
          ))}
        </View>

        <Button
          mode="contained"
          onPress={handleCreate}
          loading={loading}
          disabled={loading || !name.trim()}
          style={[styles.createButton, !name.trim() && styles.disabledButton]}
        >
          {loading ? 'Creating...' : 'Create Event'}
        </Button>
      </ScrollView>
    </SafeAreaView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#FAFAFA',
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    padding: 16,
    backgroundColor: '#FFFFFF',
    borderBottomWidth: 1,
    borderBottomColor: '#E2E8F0',
    elevation: 2,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.1,
    shadowRadius: 2,
  },
  title: {
    fontSize: 18,
    fontWeight: '600',
    color: '#2D3748',
  },
  content: {
    flex: 1,
    padding: 16,
  },
  input: {
    marginBottom: 16,
    backgroundColor: '#FFFFFF',
  },
  sectionTitle: {
    fontSize: 16,
    fontWeight: '600',
    marginBottom: 12,
    color: '#2D3748',
  },
  typeGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'space-between',
    marginBottom: 32,
  },
  typeCard: {
    width: '30%',
    aspectRatio: 1,
    backgroundColor: '#FFFFFF',
    borderRadius: 12,
    justifyContent: 'center',
    alignItems: 'center',
    marginBottom: 12,
    elevation: 2,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.1,
    shadowRadius: 2,
  },
  typeLabel: {
    fontSize: 12,
    fontWeight: '500',
    marginTop: 8,
    color: '#2D3748',
  },
  createButton: {
    marginTop: 16,
    backgroundColor: '#1A365D',
  },
  disabledButton: {
    backgroundColor: '#A0AEC0',
  },
});

export default CreateEventScreen;